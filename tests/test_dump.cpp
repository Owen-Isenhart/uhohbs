#include <obs.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "dump.hpp"

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
	int cutCalls{0};
	dump_result cutResult{dump_result_type::Success, "cut"};

	bool is_streaming_active() const override { return true; }

	dump_result execute_cut() override
	{
		++cutCalls;
		return cutResult;
	}
};

void test_coordinator_cut_path()
{
	auto bridge = std::make_unique<fake_bridge>();
	auto *bridgePtr = bridge.get();
	bridgePtr->cutResult = {dump_result_type::Success, "cut ok"};

	dump_coordinator coordinator(std::move(bridge));
	std::vector<dump_result> statuses;
	coordinator.set_status_callback([&statuses](const dump_result &result) { statuses.push_back(result); });

	const dump_result result = coordinator.request_dump();

	EXPECT_EQ(bridgePtr->cutCalls, 1);
	EXPECT_EQ(static_cast<int>(result.type), static_cast<int>(dump_result_type::Success));
	EXPECT_EQ(static_cast<int>(statuses.size()), 2);
	EXPECT_EQ(static_cast<int>(statuses[0].type), static_cast<int>(dump_result_type::InProgress));
	EXPECT_EQ(static_cast<int>(statuses[1].type), static_cast<int>(dump_result_type::Success));
}


int run_all_tests()
{
	const std::vector<test_case> tests = {
		{"coordinator cut path", test_coordinator_cut_path},
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
