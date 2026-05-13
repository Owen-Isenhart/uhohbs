#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>


enum class dump_result_type {
	InProgress,
	Success,
	Failure,
};

struct dump_result {
	dump_result_type type{dump_result_type::Failure};
	std::string message;
};

class obs_runtime_bridge {
public:
	virtual ~obs_runtime_bridge() = default;

	virtual bool is_streaming_active() const = 0;
	virtual dump_result execute_cut() = 0;
};

class obs_runtime_bridge_impl final : public obs_runtime_bridge {
public:
	bool is_streaming_active() const override;
	dump_result execute_cut() override;
};

class dump_coordinator {
public:
	using status_callback_t = std::function<void(const dump_result &)>;

	explicit dump_coordinator(
		std::unique_ptr<obs_runtime_bridge> bridge = std::make_unique<obs_runtime_bridge_impl>());

	void set_status_callback(status_callback_t callback);
	bool in_progress() const;
	dump_result request_dump();

private:
	void notify_status(const dump_result &result) const;

	std::unique_ptr<obs_runtime_bridge> bridge;
	status_callback_t statusCallback;
	std::atomic<bool> operationInProgress{false};
};
