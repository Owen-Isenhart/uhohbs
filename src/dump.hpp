#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <obs-frontend-api.h>

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
	virtual dump_result execute_cut(bool disable_delay) = 0;
};

class obs_runtime_bridge_impl final : public obs_runtime_bridge {
public:
	obs_runtime_bridge_impl();
	~obs_runtime_bridge_impl() override;

	bool is_streaming_active() const override;
	dump_result execute_cut(bool disable_delay) override;

private:
	static void frontend_event_callback(enum obs_frontend_event event, void *private_data);
	void handle_event(enum obs_frontend_event event);
	void restore_delay_if_needed();

	std::atomic<bool> is_internal_stop{false};
	std::atomic<bool> has_saved_delay{false};
	bool original_delay_enable{false};
};

class dump_coordinator {
public:
	using status_callback_t = std::function<void(const dump_result &)>;

	explicit dump_coordinator(
		std::unique_ptr<obs_runtime_bridge> bridge = std::make_unique<obs_runtime_bridge_impl>());

	void set_status_callback(status_callback_t callback);
	bool in_progress() const;
	dump_result request_dump(bool disable_delay);

private:
	void notify_status(const dump_result &result) const;

	std::unique_ptr<obs_runtime_bridge> bridge;
	status_callback_t statusCallback;
	std::atomic<bool> operationInProgress{false};
};
