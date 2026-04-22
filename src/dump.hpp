#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "dump_config.hpp"

enum class dump_result_type {
    InProgress,
    Success,
    Failure,
};

struct dump_result {
    dump_result_type type{dump_result_type::Failure};
    bool usedFallback{false};
    std::string message;
};

class obs_runtime_bridge {
public:
    virtual ~obs_runtime_bridge() = default;

    virtual bool is_streaming_active() const = 0;
    virtual bool supports_fill_rewrite() const = 0;
    virtual bool supports_replay_buffer() const = 0;
    virtual dump_result execute_cut(const dump_config &config) = 0;
    virtual dump_result execute_fill(const dump_config &config) = 0;
};

class obs_runtime_bridge_impl final : public obs_runtime_bridge {
public:
    bool is_streaming_active() const override;
    bool supports_fill_rewrite() const override;
    bool supports_replay_buffer() const override;
    dump_result execute_cut(const dump_config &config) override;
    dump_result execute_fill(const dump_config &config) override;
};

class dump_coordinator {
public:
    using status_callback_t = std::function<void(const dump_result &)>;

    explicit dump_coordinator(std::unique_ptr<obs_runtime_bridge> bridge = std::make_unique<obs_runtime_bridge_impl>());

    void set_status_callback(status_callback_t callback);
    bool in_progress() const;
    dump_result request_dump(const dump_config &config);

private:
    void notify_status(const dump_result &result) const;

    std::unique_ptr<obs_runtime_bridge> bridge;
    status_callback_t statusCallback;
    std::atomic<bool> operationInProgress{false};
};