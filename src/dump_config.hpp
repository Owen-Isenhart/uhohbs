#pragma once

#include <obs-module.h>

enum class pipeline_target {
	StreamDelay = 0,
	ReplayBuffer = 1,
};

class dump_config {
public:
	static constexpr const char *kPipelineTargetKey = "pipeline_target";

	dump_config() = default;

	pipeline_target get_pipeline_target() const { return pipelineTarget; }

	void set_pipeline_target(pipeline_target target) { pipelineTarget = target; }

	void save_settings(obs_data_t *data) const
	{
		if (!data) {
			return;
		}

		obs_data_set_int(data, kPipelineTargetKey, static_cast<long long>(pipelineTarget));
	}

	void load_settings(obs_data_t *data)
	{
		if (!data) {
			return;
		}

		if (obs_data_has_user_value(data, kPipelineTargetKey)) {
			const auto pipelineRaw = obs_data_get_int(data, kPipelineTargetKey);
			set_pipeline_target(pipelineRaw == static_cast<long long>(pipeline_target::ReplayBuffer)
						    ? pipeline_target::ReplayBuffer
						    : pipeline_target::StreamDelay);
		}
	}

private:
	pipeline_target pipelineTarget{pipeline_target::StreamDelay};
};
