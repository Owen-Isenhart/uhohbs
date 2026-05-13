#include "dump.hpp"

#include <obs-frontend-api.h>
#include <obs.h>
#include <plugin-support.h>
#include <util/platform.h>

#include <cstdint>
#include <cctype>
#include <optional>
#include <type_traits>
#include <utility>

namespace {
constexpr const char *kInProgressMsg = "Dump operation started";
constexpr const char *kNotStreamingMsg = "Streaming is not active";
constexpr const char *kStreamingOutputUnavailableMsg = "Streaming output is unavailable";
constexpr const char *kStreamStopTimeoutMsg = "Timed out waiting for stream to stop during delay cut";
constexpr const char *kStreamRestartFailedMsg = "Delay cut stopped stream, but restart failed";
constexpr const char *kReplayBufferNotActiveMsg = "Replay buffer is not active";
constexpr const char *kReplayBufferStopTimeoutMsg = "Timed out waiting for replay buffer to stop";
constexpr const char *kReplayBufferRestartFailedMsg = "Replay buffer restart failed";
constexpr const char *kCutSuccessMsg = "Delay cut triggered";
constexpr const char *kReplayCutSuccessMsg = "Replay buffer restarted to drop buffered content";
constexpr const char *kFillSuccessMsg = "Delay filled with censor content and returned to live scene";
constexpr const char *kFillInvalidTargetMsg = "Fill target source or scene was not found";
constexpr const char *kFillSourceCreateFailedMsg = "Failed to create temporary fill source";
constexpr const char *kStreamDelayInactiveMsg =
	"OBS stream delay is inactive; enable stream delay in OBS output settings";
constexpr const char *kBusyMsg = "A dump operation is already in progress";

template<typename Fn> auto run_on_ui_thread(Fn &&fn) -> std::invoke_result_t<Fn>
{
	using Result = std::invoke_result_t<Fn>;
	if (obs_in_task_thread(OBS_TASK_UI)) {
		if constexpr (std::is_void_v<Result>) {
			fn();
			return;
		} else {
			return fn();
		}
	}

	if constexpr (std::is_void_v<Result>) {
		struct task_payload {
			Fn fn;
		};
		task_payload payload{std::forward<Fn>(fn)};
		obs_queue_task(
			OBS_TASK_UI,
			[](void *param) {
				auto *payload = static_cast<task_payload *>(param);
				payload->fn();
			},
			&payload, true);
	} else {
		struct task_payload {
			Fn fn;
			std::optional<Result> result;
		};
		task_payload payload{std::forward<Fn>(fn), std::nullopt};
		obs_queue_task(
			OBS_TASK_UI,
			[](void *param) {
				auto *payload = static_cast<task_payload *>(param);
				payload->result = payload->fn();
			},
			&payload, true);
		return std::move(*payload.result);
	}
}

std::uint32_t default_fill_rgb()
{
	return 0xFF0000;
}

bool is_hex_color(const std::string &value)
{
	if (value.size() != 7 || value[0] != '#') {
		return false;
	}

	for (size_t i = 1; i < value.size(); ++i) {
		if (!std::isxdigit(static_cast<unsigned char>(value[i]))) {
			return false;
		}
	}

	return true;
}

std::uint32_t parse_fill_rgb(const std::string &colorHex)
{
	if (!is_hex_color(colorHex)) {
		return default_fill_rgb();
	}

	try {
		return static_cast<std::uint32_t>(std::stoul(colorHex.substr(1), nullptr, 16));
	} catch (...) {
		return default_fill_rgb();
	}
}

void get_fill_dimensions(long long &width, long long &height)
{
	width = 1920;
	height = 1080;

	obs_video_info ovi{};
	if (obs_get_video_info(&ovi) && ovi.base_width > 0 && ovi.base_height > 0) {
		width = static_cast<long long>(ovi.base_width);
		height = static_cast<long long>(ovi.base_height);
	}
}

bool wait_until(std::function<bool()> predicate, uint32_t timeoutMs)
{
	constexpr uint32_t stepMs = 50;
	uint32_t waitedMs = 0;
	while (waitedMs < timeoutMs) {
		if (predicate()) {
			return true;
		}
		os_sleep_ms(stepMs);
		waitedMs += stepMs;
	}
	return predicate();
}

obs_source_t *create_color_source(const dump_config &config)
{
	obs_data_t *settings = obs_data_create();
	if (!settings) {
		return nullptr;
	}

	const std::string colorHex = config.get_fill_color_hex().empty() ? "#ff0000" : config.get_fill_color_hex();
	const std::uint32_t rgb = parse_fill_rgb(colorHex);

	long long width = 1920;
	long long height = 1080;
	get_fill_dimensions(width, height);

	obs_data_set_int(settings, "color", static_cast<long long>(0xFF000000 | rgb));
	obs_data_set_int(settings, "width", width);
	obs_data_set_int(settings, "height", height);

	obs_source_t *source = obs_source_create_private("color_source", "uhohbs_fill_color", settings);
	obs_data_release(settings);
	return source;
}

obs_source_t *resolve_fill_source(const dump_config &config, obs_source_t **ownedCreatedSource)
{
	if (ownedCreatedSource) {
		*ownedCreatedSource = nullptr;
	}

	if (config.get_fill_type() == dump_mode::FillType::Color) {
		obs_source_t *colorSource = create_color_source(config);
		if (ownedCreatedSource) {
			*ownedCreatedSource = colorSource;
		}
		return colorSource;
	}

	if (config.get_fill_target_name().empty()) {
		return nullptr;
	}

	return obs_get_source_by_name(config.get_fill_target_name().c_str());
}

struct fill_scene_resources {
	obs_scene_t *scene{nullptr};
	obs_source_t *ownedFillSource{nullptr};
};

fill_scene_resources build_fill_scene(const dump_config &config)
{
	fill_scene_resources result{};

	if (config.get_fill_type() == dump_mode::FillType::Scene) {
		if (config.get_fill_target_name().empty()) {
			return result;
		}
		result.scene = obs_get_scene_by_name(config.get_fill_target_name().c_str());
		return result;
	}

	obs_source_t *fillSource = resolve_fill_source(config, &result.ownedFillSource);
	if (!fillSource) {
		return result;
	}

	obs_scene_t *fillScene = obs_scene_create_private("uhohbs_fill_scene");
	if (!fillScene) {
		if (result.ownedFillSource) {
			obs_source_release(result.ownedFillSource);
			result.ownedFillSource = nullptr;
		} else {
			obs_source_release(fillSource);
		}
		return result;
	}

	obs_scene_add(fillScene, fillSource);
	if (!result.ownedFillSource) {
		obs_source_release(fillSource);
	}

	result.scene = fillScene;
	return result;
}
} // namespace

bool obs_runtime_bridge_impl::is_streaming_active() const
{
	return run_on_ui_thread([]() { return obs_frontend_streaming_active(); });
}

bool obs_runtime_bridge_impl::supports_fill_rewrite() const
{
	return true;
}

bool obs_runtime_bridge_impl::supports_replay_buffer() const
{
	return true;
}

dump_result obs_runtime_bridge_impl::execute_cut(const dump_config &config)
{
	if (config.get_pipeline_target() == pipeline_target::ReplayBuffer) {
		if (!run_on_ui_thread([]() { return obs_frontend_replay_buffer_active(); })) {
			return {dump_result_type::Failure, false, kReplayBufferNotActiveMsg};
		}

		run_on_ui_thread([]() { obs_frontend_replay_buffer_stop(); });
		if (!wait_until([]() { return !run_on_ui_thread([]() { return obs_frontend_replay_buffer_active(); }); },
				3000)) {
			return {dump_result_type::Failure, false, kReplayBufferStopTimeoutMsg};
		}

		run_on_ui_thread([]() { obs_frontend_replay_buffer_start(); });
		if (!wait_until([]() { return run_on_ui_thread([]() { return obs_frontend_replay_buffer_active(); }); },
				5000)) {
			return {dump_result_type::Failure, false, kReplayBufferRestartFailedMsg};
		}

		obs_log(LOG_INFO, "%s", kReplayCutSuccessMsg);
		return {dump_result_type::Success, false, kReplayCutSuccessMsg};
	}

	if (!is_streaming_active()) {
		return {dump_result_type::Failure, false, kNotStreamingMsg};
	}

	obs_output_t *streamOutput = run_on_ui_thread([]() { return obs_frontend_get_streaming_output(); });
	if (!streamOutput) {
		return {dump_result_type::Failure, false, kStreamingOutputUnavailableMsg};
	}

	const uint32_t activeDelaySeconds =
		run_on_ui_thread([streamOutput]() { return obs_output_get_active_delay(streamOutput); });
	if (activeDelaySeconds == 0) {
		run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });
		return {dump_result_type::Failure, false, kStreamDelayInactiveMsg};
	}

	run_on_ui_thread([streamOutput]() { obs_output_force_stop(streamOutput); });
	run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });

	if (!wait_until([this]() { return !is_streaming_active(); }, 5000)) {
		return {dump_result_type::Failure, false, kStreamStopTimeoutMsg};
	}

	run_on_ui_thread([]() { obs_frontend_streaming_start(); });
	if (!wait_until([]() { return run_on_ui_thread([]() { return obs_frontend_streaming_active(); }); }, 10000)) {
		return {dump_result_type::Failure, false, kStreamRestartFailedMsg};
	}

	obs_log(LOG_INFO, "%s", kCutSuccessMsg);
	return {dump_result_type::Success, false, kCutSuccessMsg};
}

dump_result obs_runtime_bridge_impl::execute_fill(const dump_config &config)
{
	if (config.get_pipeline_target() == pipeline_target::ReplayBuffer) {
		return execute_cut(config);
	}

	if (!is_streaming_active()) {
		return {dump_result_type::Failure, false, kNotStreamingMsg};
	}

	obs_output_t *streamOutput = run_on_ui_thread([]() { return obs_frontend_get_streaming_output(); });
	if (!streamOutput) {
		return {dump_result_type::Failure, false, kStreamingOutputUnavailableMsg};
	}

	const uint32_t activeDelaySeconds =
		run_on_ui_thread([streamOutput]() { return obs_output_get_active_delay(streamOutput); });
	run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });
	if (activeDelaySeconds == 0) {
		return {dump_result_type::Failure, false, kStreamDelayInactiveMsg};
	}

	obs_source_t *currentScene = run_on_ui_thread([]() { return obs_frontend_get_current_scene(); });
	if (!currentScene) {
		return {dump_result_type::Failure, false, kFillInvalidTargetMsg};
	}

	fill_scene_resources fillScene = run_on_ui_thread([&config]() { return build_fill_scene(config); });
	if (!fillScene.scene) {
		run_on_ui_thread([currentScene]() { obs_source_release(currentScene); });
		return {dump_result_type::Failure, false, kFillSourceCreateFailedMsg};
	}

	run_on_ui_thread([scene = fillScene.scene]() { obs_frontend_set_current_scene(obs_scene_get_source(scene)); });
	os_sleep_ms(static_cast<uint32_t>(config.get_delay_seconds()) * 1000U);
	run_on_ui_thread([currentScene]() { obs_frontend_set_current_scene(currentScene); });

	run_on_ui_thread([&fillScene, currentScene]() {
		if (fillScene.ownedFillSource) {
			obs_source_release(fillScene.ownedFillSource);
		}
		if (fillScene.scene) {
			obs_scene_release(fillScene.scene);
		}
		obs_source_release(currentScene);
	});

	obs_log(LOG_INFO, "%s", kFillSuccessMsg);
	return {dump_result_type::Success, false, kFillSuccessMsg};
}

dump_coordinator::dump_coordinator(std::unique_ptr<obs_runtime_bridge> bridge_) : bridge(std::move(bridge_)) {}

void dump_coordinator::set_status_callback(status_callback_t callback)
{
	statusCallback = std::move(callback);
}

bool dump_coordinator::in_progress() const
{
	return operationInProgress.load();
}

dump_result dump_coordinator::request_dump(const dump_config &config)
{
	if (operationInProgress.exchange(true)) {
		const dump_result busyResult{dump_result_type::Failure, false, kBusyMsg};
		notify_status(busyResult);
		return busyResult;
	}

	const dump_result inProgressResult{dump_result_type::InProgress, false, kInProgressMsg};
	notify_status(inProgressResult);

	dump_result result;
	if (config.get_pipeline_target() == pipeline_target::ReplayBuffer && !bridge->supports_replay_buffer()) {
		result = {dump_result_type::Failure, false, kReplayBufferNotActiveMsg};
	} else if (config.get_mode() == dump_mode::Mode::Cut) {
		result = bridge->execute_cut(config);
	} else if (bridge->supports_fill_rewrite()) {
		result = bridge->execute_fill(config);
	} else {
		result = bridge->execute_cut(config);
		result.usedFallback = true;
	}

	operationInProgress.store(false);
	notify_status(result);
	return result;
}

void dump_coordinator::notify_status(const dump_result &result) const
{
	if (statusCallback) {
		statusCallback(result);
	}
}