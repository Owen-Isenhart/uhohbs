#include <string>

struct dump_mode {
    enum class Mode {
        Cut,
        Fill
    };

    enum class FillType {
        None,
        Source,
        Scene,
        Color
    };
};

class dump_config {
    public:
        // constructors
        dump_config() {
            censor_name = "";
            delaySeconds = 5;
            mode = dump_mode::Mode::Cut;
            fillType = dump_mode::FillType::None;
        }
        dump_config(u_int16_t delaySeconds) {
            censor_name = "";
            this->delaySeconds = delaySeconds;
            mode = dump_mode::Mode::Cut;
            fillType = dump_mode::FillType::None;
        }
        dump_config(std::string censorSceneName, u_int16_t delaySeconds, dump_mode::Mode mode, dump_mode::FillType fillType = dump_mode::FillType::None) {
            this->censor_name = censorSceneName;
            this->delaySeconds = delaySeconds;
            this->mode = mode;
            this->fillType = fillType; // 1 for source, 2 for scene, 3 for color
        }

        // getters
        std::string get_censor_name() {
            return censor_name;
        }
        u_int16_t get_delay_seconds() {
            return delaySeconds;
        }
        dump_mode::Mode get_mode() {
            return mode;
        }
        dump_mode::FillType get_fill_type() {
            return fillType;
        }

        // setters
        void set_censor_name(std::string name) {
            censor_name = name;
        }
        void set_delay_seconds(u_int16_t seconds) {
            delaySeconds = seconds;
        }
        void set_mode(dump_mode::Mode mode) {
            this->mode = mode;
        }
        void set_fill_type(dump_mode::FillType fillType) {
            this->fillType = fillType;
        }

        void save_settings() {
            // this is gonna deal with the obs data object
        }
    private:
        std::string censor_name; // can be a scene, source, or color
        u_int16_t delaySeconds;
        dump_mode::Mode mode;
        dump_mode::FillType fillType;
};