#include <obs.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "dump.hpp"
#include "dump_config.hpp"

namespace {
class test_failure final : public std::runtime_error {
public:
	explicit test_failure(const std::string &message) : std::runtime_error(message) {}
};

inline std::string to_string_fallback(const std::string &value)
{
	return value;
}

inline std::string to_string_fallback(const char *value)
{
	return value ? std::string(value) : std::string("<null>");
}

template<typename T> std::string to_string_fallback(const T &value)
{
	std::ostringstream stream;
	stream << value;
	return stream.str();
}

template<typename Actual, typename Expected>
void expect_equal(const Actual &actual, const Expected &expected, const char *actualExpr, const char *expectedExpr,
		  const char *file, int line)
{
	if (!(actual == expected)) {
		std::ostringstream stream;
		stream << file << ":" << line << " expected " << actualExpr << " == " << expectedExpr
		       << " (actual=" << to_string_fallback(actual) << ", expected=" << to_string_fallback(expected)
		       << ")";
		throw test_failure(stream.str());
	}
}

void expect_true(bool value, const char *expr, const char *file, int line)
{
	if (!value) {
		std::ostringstream stream;
		stream << file << ":" << line << " expected " << expr << " to be true";
		throw test_failure(stream.str());
	}
}

#define EXPECT_EQ(actual, expected) expect_equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)
#define EXPECT_TRUE(expr) expect_true((expr), #expr, __FILE__, __LINE__)

struct test_case {
	const char *name;
	std::function<void()> fn;
};

struct fake_bridge final : public obs_runtime_bridge {
	bool supportsFill{true};
	bool supportsReplay{true};
	int cutCalls{0};
	int fillCalls{0};
	dump_result cutResult{dump_result_type::Success, false, "cut"};
	dump_result fillResult{dump_result_type::Success, false, "fill"};

	bool is_streaming_active() const override { return true; }
	bool supports_fill_rewrite() const override { return supportsFill; }
	bool supports_replay_buffer() const override { return supportsReplay; }

	dump_result execute_cut(const dump_config &) override
	{
		++cutCalls;
		return cutResult;
	}

	dump_result execute_fill(const dump_config &) override
	{
		++fillCalls;
		return fillResult;
	}
};

void test_delay_clamping()
{
	dump_config config;
	config.set_delay_seconds(0);
	EXPECT_EQ(config.get_delay_seconds(), static_cast<std::uint16_t>(1));
	config.set_delay_seconds(301);
	EXPECT_EQ(config.get_delay_seconds(), static_cast<std::uint16_t>(300));
	config.set_delay_seconds(42);
	EXPECT_EQ(config.get_delay_seconds(), static_cast<std::uint16_t>(42));
}

void test_load_empty_fill_color_defaults()
{
	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, dump_config::kFillColorHexKey, "");

	dump_config config;
	config.load_settings(data);

	EXPECT_EQ(config.get_fill_color_hex(), std::string("#ff0000"));
	obs_data_release(data);
}

void test_load_enums_and_values()
{
	obs_data_t *data = obs_data_create();
	obs_data_set_int(data, dump_config::kDelaySecondsKey, 17);
	obs_data_set_int(data, dump_config::kModeKey, static_cast<long long>(dump_mode::Mode::Fill));
	obs_data_set_int(data, dump_config::kFillTypeKey, 99);
	obs_data_set_string(data, dump_config::kFillTargetNameKey, "Camera");
	obs_data_set_string(data, dump_config::kFillColorHexKey, "#00ff00");
	obs_data_set_int(data, dump_config::kPipelineTargetKey, static_cast<long long>(pipeline_target::ReplayBuffer));

	dump_config config;
	config.load_settings(data);

	EXPECT_EQ(config.get_delay_seconds(), static_cast<std::uint16_t>(17));
	EXPECT_EQ(static_cast<int>(config.get_mode()), static_cast<int>(dump_mode::Mode::Fill));
	EXPECT_EQ(static_cast<int>(config.get_fill_type()), static_cast<int>(dump_mode::FillType::Color));
	EXPECT_EQ(config.get_fill_target_name(), std::string("Camera"));
	EXPECT_EQ(config.get_fill_color_hex(), std::string("#00ff00"));
	EXPECT_EQ(static_cast<int>(config.get_pipeline_target()), static_cast<int>(pipeline_target::ReplayBuffer));

	obs_data_release(data);
}

void test_save_roundtrip()
{
	dump_config config;
	config.set_delay_seconds(123);
	config.set_mode(dump_mode::Mode::Fill);
	config.set_fill_type(dump_mode::FillType::Source);
	config.set_fill_target_name("Mic");
	config.set_fill_color_hex("#112233");
	config.set_pipeline_target(pipeline_target::ReplayBuffer);

	obs_data_t *data = obs_data_create();
	config.save_settings(data);

	dump_config loaded;
	loaded.load_settings(data);

	EXPECT_EQ(loaded.get_delay_seconds(), static_cast<std::uint16_t>(123));
	EXPECT_EQ(static_cast<int>(loaded.get_mode()), static_cast<int>(dump_mode::Mode::Fill));
	EXPECT_EQ(static_cast<int>(loaded.get_fill_type()), static_cast<int>(dump_mode::FillType::Source));
	EXPECT_EQ(loaded.get_fill_target_name(), std::string("Mic"));
	EXPECT_EQ(loaded.get_fill_color_hex(), std::string("#112233"));
	EXPECT_EQ(static_cast<int>(loaded.get_pipeline_target()), static_cast<int>(pipeline_target::ReplayBuffer));

	obs_data_release(data);
}

void test_coordinator_cut_path()
{
	auto bridge = std::make_unique<fake_bridge>();
	auto *bridgePtr = bridge.get();
	bridgePtr->cutResult = {dump_result_type::Success, false, "cut ok"};

	dump_coordinator coordinator(std::move(bridge));
	std::vector<dump_result> statuses;
	coordinator.set_status_callback([&statuses](const dump_result &result) { statuses.push_back(result); });

	dump_config config;
	config.set_mode(dump_mode::Mode::Cut);

	const dump_result result = coordinator.request_dump(config);

	EXPECT_EQ(bridgePtr->cutCalls, 1);
	EXPECT_EQ(bridgePtr->fillCalls, 0);
	EXPECT_EQ(static_cast<int>(result.type), static_cast<int>(dump_result_type::Success));
	EXPECT_EQ(static_cast<int>(statuses.size()), 2);
	EXPECT_EQ(static_cast<int>(statuses[0].type), static_cast<int>(dump_result_type::InProgress));
	EXPECT_EQ(static_cast<int>(statuses[1].type), static_cast<int>(dump_result_type::Success));
}

void test_coordinator_fill_fallback_to_cut()
{
	auto bridge = std::make_unique<fake_bridge>();
	auto *bridgePtr = bridge.get();
	bridgePtr->supportsFill = false;
	bridgePtr->cutResult = {dump_result_type::Success, false, "cut ok"};

	dump_coordinator coordinator(std::move(bridge));

	dump_config config;
	config.set_mode(dump_mode::Mode::Fill);

	const dump_result result = coordinator.request_dump(config);

	EXPECT_EQ(bridgePtr->cutCalls, 1);
	EXPECT_EQ(bridgePtr->fillCalls, 0);
	EXPECT_TRUE(result.usedFallback);
}

void test_replay_buffer_not_supported()
{
	auto bridge = std::make_unique<fake_bridge>();
	auto *bridgePtr = bridge.get();
	bridgePtr->supportsReplay = false;

	dump_coordinator coordinator(std::move(bridge));

	dump_config config;
	config.set_mode(dump_mode::Mode::Cut);
	config.set_pipeline_target(pipeline_target::ReplayBuffer);

	const dump_result result = coordinator.request_dump(config);

	EXPECT_EQ(static_cast<int>(result.type), static_cast<int>(dump_result_type::Failure));
	EXPECT_EQ(result.message, std::string("Replay buffer is not active"));
	EXPECT_EQ(bridgePtr->cutCalls, 0);
	EXPECT_EQ(bridgePtr->fillCalls, 0);
}

int run_all_tests()
{
	const std::vector<test_case> tests = {
		{"delay clamping", test_delay_clamping},
		{"empty fill color defaults", test_load_empty_fill_color_defaults},
		{"load enums and values", test_load_enums_and_values},
		{"save roundtrip", test_save_roundtrip},
		{"coordinator cut path", test_coordinator_cut_path},
		{"coordinator fill fallback", test_coordinator_fill_fallback_to_cut},
		{"replay buffer not supported", test_replay_buffer_not_supported},
	};

	int failures = 0;
	for (const auto &test : tests) {
		try {
			test.fn();
		} catch (const test_failure &failure) {
			std::cerr << "FAIL: " << test.name << " - " << failure.what() << '\n';
			++failures;
		} catch (const std::exception &ex) {
			std::cerr << "ERROR: " << test.name << " - " << ex.what() << '\n';
			++failures;
		} catch (...) {
			std::cerr << "ERROR: " << test.name << " - unknown exception\n";
			++failures;
		}
	}

	if (failures > 0) {
		std::cerr << failures << " test(s) failed\n";
	}

	return failures == 0 ? 0 : 1;
}
} // namespace

int main()
{
	return run_all_tests();
}
