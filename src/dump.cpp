#include "dump.hpp"

#include <obs-frontend-api.h>
#include <obs.h>
#include <plugin-support.h>
#include <util/config-file.h>
#include <util/platform.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace {
constexpr const char *kInProgressMsg = "Dump operation started";
constexpr const char *kNotStreamingMsg = "Streaming is not active";
constexpr const char *kStreamingOutputUnavailableMsg = "Streaming output is unavailable";
constexpr const char *kStreamStopTimeoutMsg = "Timed out waiting for stream to stop during delay cut";
constexpr const char *kStreamRestartFailedMsg = "Delay cut stopped stream, but restart failed";
constexpr const char *kCutSuccessMsg = "Delay cut triggered";
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

class stream_event_waiter {
	std::mutex m;
	std::condition_variable cv;

	static void callback(enum obs_frontend_event event, void *private_data)
	{
		if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED || event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
			auto *waiter = static_cast<stream_event_waiter *>(private_data);
			std::lock_guard<std::mutex> lock(waiter->m);
			waiter->cv.notify_all();
		}
	}

public:
	stream_event_waiter()
	{
		run_on_ui_thread([this]() { obs_frontend_add_event_callback(callback, this); });
	}

	~stream_event_waiter()
	{
		run_on_ui_thread([this]() { obs_frontend_remove_event_callback(callback, this); });
	}

	bool wait_until(std::function<bool()> predicate, uint32_t timeoutMs)
	{
		std::unique_lock<std::mutex> lock(m);
		auto now = std::chrono::steady_clock::now();
		auto end_time = now + std::chrono::milliseconds(timeoutMs);

		while (now < end_time) {
			if (predicate()) {
				return true;
			}
			cv.wait_for(lock, std::chrono::milliseconds(250));
			now = std::chrono::steady_clock::now();
		}
		return predicate();
	}
};
} // namespace

obs_runtime_bridge_impl::obs_runtime_bridge_impl()
{
	run_on_ui_thread([this]() { obs_frontend_add_event_callback(frontend_event_callback, this); });
}

obs_runtime_bridge_impl::~obs_runtime_bridge_impl()
{
	run_on_ui_thread([this]() { obs_frontend_remove_event_callback(frontend_event_callback, this); });
	restore_delay_if_needed();
}

void obs_runtime_bridge_impl::frontend_event_callback(enum obs_frontend_event event, void *private_data)
{
	static_cast<obs_runtime_bridge_impl *>(private_data)->handle_event(event);
}

void obs_runtime_bridge_impl::handle_event(enum obs_frontend_event event)
{
	if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		if (!is_internal_stop) {
			restore_delay_if_needed();
		}
	}
}

void obs_runtime_bridge_impl::restore_delay_if_needed()
{
	if (has_saved_delay) {
		run_on_ui_thread([this]() {
			config_t *profile = obs_frontend_get_profile_config();
			if (profile) {
				config_set_bool(profile, "Output", "DelayEnable", original_delay_enable);
				has_saved_delay = false;
			}
		});
	}
}

bool obs_runtime_bridge_impl::is_streaming_active() const
{
	return run_on_ui_thread([]() { return obs_frontend_streaming_active(); });
}

dump_result obs_runtime_bridge_impl::execute_cut(bool disable_delay)
{
	if (!is_streaming_active()) {
		return {dump_result_type::Failure, kNotStreamingMsg};
	}

	obs_output_t *streamOutput = run_on_ui_thread([]() { return obs_frontend_get_streaming_output(); });
	if (!streamOutput) {
		return {dump_result_type::Failure, kStreamingOutputUnavailableMsg};
	}

	const uint32_t activeDelaySeconds =
		run_on_ui_thread([streamOutput]() { return obs_output_get_active_delay(streamOutput); });
	if (activeDelaySeconds == 0) {
		run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });
		return {dump_result_type::Failure, kStreamDelayInactiveMsg};
	}

	stream_event_waiter waiter;

	is_internal_stop = true;
	run_on_ui_thread([streamOutput]() { obs_output_force_stop(streamOutput); });

	const auto stream_stopped = [streamOutput]() {
		return run_on_ui_thread([streamOutput]() {
			return !obs_output_active(streamOutput) && !obs_frontend_streaming_active();
		});
	};

	if (!waiter.wait_until(stream_stopped, 5000)) {
		run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });
		is_internal_stop = false;
		return {dump_result_type::Failure, kStreamStopTimeoutMsg};
	}

	run_on_ui_thread([streamOutput]() { obs_output_release(streamOutput); });

	if (disable_delay && !has_saved_delay) {
		run_on_ui_thread([this]() {
			config_t *profile = obs_frontend_get_profile_config();
			if (profile) {
				original_delay_enable = config_get_bool(profile, "Output", "DelayEnable");
				config_set_bool(profile, "Output", "DelayEnable", false);
				has_saved_delay = true;
			}
		});
	}

	const uint32_t restartTimeoutMs = std::max<uint32_t>(10000, (activeDelaySeconds + 5) * 1000u);

	run_on_ui_thread([]() { obs_frontend_streaming_start(); });
	if (!waiter.wait_until([]() { return run_on_ui_thread([]() { return obs_frontend_streaming_active(); }); },
			       restartTimeoutMs)) {
		is_internal_stop = false;
		return {dump_result_type::Failure, kStreamRestartFailedMsg};
	}

	is_internal_stop = false;
	obs_log(LOG_INFO, "%s", kCutSuccessMsg);
	return {dump_result_type::Success, kCutSuccessMsg};
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

dump_result dump_coordinator::request_dump(bool disable_delay)
{
	if (operationInProgress.exchange(true)) {
		const dump_result busyResult{dump_result_type::Failure, kBusyMsg};
		notify_status(busyResult);
		return busyResult;
	}

	const dump_result inProgressResult{dump_result_type::InProgress, kInProgressMsg};
	notify_status(inProgressResult);

	dump_result result;
	result = bridge->execute_cut(disable_delay);

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
