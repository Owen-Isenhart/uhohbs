#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

#include <obs-module.h>

struct dump_mode {
    enum class Mode {
        Cut = 0,
        Fill = 1,
    };

    enum class FillType {
        Color = 0,
        Source = 1,
        Scene = 2,
    };
};

enum class pipeline_target {
    StreamDelay = 0,
    ReplayBuffer = 1,
};

class dump_config {
public:
    static constexpr const char *kDelaySecondsKey = "delay_seconds";
    static constexpr const char *kModeKey = "mode";
    static constexpr const char *kFillTypeKey = "fill_type";
    static constexpr const char *kFillTargetNameKey = "fill_target_name";
    static constexpr const char *kFillColorHexKey = "fill_color_hex";
    static constexpr const char *kPipelineTargetKey = "pipeline_target";

    dump_config() = default;

    std::uint16_t get_delay_seconds() const { return delaySeconds; }
    dump_mode::Mode get_mode() const { return mode; }
    dump_mode::FillType get_fill_type() const { return fillType; }
    const std::string &get_fill_target_name() const { return fillTargetName; }
    const std::string &get_fill_color_hex() const { return fillColorHex; }
    pipeline_target get_pipeline_target() const { return pipelineTarget; }

    void set_delay_seconds(std::uint16_t seconds)
    {
        delaySeconds = std::clamp<std::uint16_t>(seconds, 1, 300);
    }
    void set_mode(dump_mode::Mode newMode) { mode = newMode; }
    void set_fill_type(dump_mode::FillType newFillType) { fillType = newFillType; }
    void set_fill_target_name(std::string name) { fillTargetName = std::move(name); }
    void set_fill_color_hex(std::string color) { fillColorHex = std::move(color); }
    void set_pipeline_target(pipeline_target target) { pipelineTarget = target; }

    void save_settings(obs_data_t *data) const
    {
        if (!data) {
            return;
        }

        obs_data_set_int(data, kDelaySecondsKey, static_cast<long long>(delaySeconds));
        obs_data_set_int(data, kModeKey, static_cast<long long>(mode));
        obs_data_set_int(data, kFillTypeKey, static_cast<long long>(fillType));
        obs_data_set_string(data, kFillTargetNameKey, fillTargetName.c_str());
        obs_data_set_string(data, kFillColorHexKey, fillColorHex.c_str());
        obs_data_set_int(data, kPipelineTargetKey, static_cast<long long>(pipelineTarget));
    }

    void load_settings(obs_data_t *data)
    {
        if (!data) {
            return;
        }

        if (obs_data_has_user_value(data, kDelaySecondsKey)) {
            set_delay_seconds(static_cast<std::uint16_t>(obs_data_get_int(data, kDelaySecondsKey)));
        }

        if (obs_data_has_user_value(data, kModeKey)) {
            const auto modeRaw = obs_data_get_int(data, kModeKey);
            set_mode(modeRaw == static_cast<long long>(dump_mode::Mode::Fill) ? dump_mode::Mode::Fill : dump_mode::Mode::Cut);
        }

        if (obs_data_has_user_value(data, kFillTypeKey)) {
            const auto fillTypeRaw = obs_data_get_int(data, kFillTypeKey);
            switch (fillTypeRaw) {
            case static_cast<long long>(dump_mode::FillType::Source):
                set_fill_type(dump_mode::FillType::Source);
                break;
            case static_cast<long long>(dump_mode::FillType::Scene):
                set_fill_type(dump_mode::FillType::Scene);
                break;
            case static_cast<long long>(dump_mode::FillType::Color):
            default:
                set_fill_type(dump_mode::FillType::Color);
                break;
            }
        }

        if (obs_data_has_user_value(data, kFillTargetNameKey)) {
            set_fill_target_name(obs_data_get_string(data, kFillTargetNameKey));
        }

        if (obs_data_has_user_value(data, kFillColorHexKey)) {
            set_fill_color_hex(obs_data_get_string(data, kFillColorHexKey));
        }

        if (obs_data_has_user_value(data, kPipelineTargetKey)) {
            const auto pipelineRaw = obs_data_get_int(data, kPipelineTargetKey);
            set_pipeline_target(pipelineRaw == static_cast<long long>(pipeline_target::ReplayBuffer)
                           ? pipeline_target::ReplayBuffer
                           : pipeline_target::StreamDelay);
        }

        if (fillColorHex.empty()) {
            fillColorHex = "#ff0000";
        }
    }

private:
    std::uint16_t delaySeconds{5};
    dump_mode::Mode mode{dump_mode::Mode::Cut};
    dump_mode::FillType fillType{dump_mode::FillType::Color};
    std::string fillTargetName;
    std::string fillColorHex{"#ff0000"};
    pipeline_target pipelineTarget{pipeline_target::StreamDelay};
};