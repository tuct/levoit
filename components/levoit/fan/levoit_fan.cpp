#include "levoit_fan.h"
#include "../levoit.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace levoit
    {

        static const char *const TAG = "levoit.fan";
        struct ModeMap
        {
            int device;
            const char *preset;
        };

        static const ModeMap MODE_MAP[] = {
            {0, "Manual"},
            {1, "Sleep"},
            {2, "Auto"},
            {5, "Pet"},
        };
        static int preset_to_device_mode(const std::string &preset)
        {
            for (auto &m : MODE_MAP)
                if (preset == m.preset)
                    return m.device;
            return -1;
        }

        static const char *device_mode_to_preset(int mode)
        {
            for (auto &m : MODE_MAP)
                if (mode == m.device)
                    return m.preset;
            return nullptr;
        }
        void LevoitFan::setup()
        {
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->apply(*this);
            }
            // based on model, set capabilities
            std::set<std::string> preset_modes;
            for (const auto &m : MODE_MAP)
            {
                preset_modes.insert(m.preset);
            }
            this->speed_count_ = 4;

            // Construct traits
            this->traits_ =
                fan::FanTraits(this->has_oscillating_, this->speed_count_ > 0, this->has_direction_, this->speed_count_);
            this->traits_.set_supported_preset_modes(preset_modes);
        }

        void LevoitFan::dump_config() { LOG_FAN("", "Levoit Fan", this); }

        void LevoitFan::control(const fan::FanCall &call)
        {
            // Start with sentinels = "no change"
            int power_cmd = -1;
            int speed_cmd = -1;
            int mode_cmd = -1;

            // Track current values
            const bool cur_state = this->state;
            const int cur_speed = this->speed;
            const std::string cur_preset = this->preset_mode;

            // ---- power ----
            if (call.get_state().has_value())
            {
                bool new_state = *call.get_state();
                if (new_state != cur_state)
                {
                    this->state = new_state;
                    power_cmd = new_state ? 1 : 0;
                }
            }

            // ---- speed ----
            if (call.get_speed().has_value() && (this->speed_count_ > 0))
            {
                int new_speed = *call.get_speed();
                if (new_speed != cur_speed)
                {
                    this->speed = new_speed;
                    speed_cmd = new_speed;
                }
            }

            // ---- preset/mode ----
            const auto preset = call.get_preset_mode(); // your API returns std::string
            if (!preset.empty() && preset != cur_preset)
            {
                this->preset_mode = preset;
                mode_cmd = preset_to_device_mode(this->preset_mode); // returns -1 if unknown
            }

            if (!parent_)
            {
                ESP_LOGW(TAG, "No parent set for fan");
                this->publish_state(); // still reflect local changes if any
                return;
            }

            // Only sends fields != -1 (as long as your parent respects sentinels)
            parent_->on_fan_command(power_cmd, speed_cmd, mode_cmd);

            this->publish_state();
        }

        void LevoitFan::apply_device_status(int power, int speed_level, int mode)
        {

            bool dirty = false;
            // power: -1 = keep current
            if (power != -1)
            {   
                if(this->state != (power == 1))
                    dirty = true;
                this->state = (power == 1);
            }

            // speed_level: -1 = keep current
            if (speed_level != -1 && this->speed_count_ > 0)
            {
                if (speed_level < 1)
                    speed_level = 1;
                if (speed_level > this->speed_count_)
                    speed_level = this->speed_count_;


                if(this->speed != speed_level)
                    dirty = true;
                this->speed = speed_level;
            }

            const char *preset = device_mode_to_preset(mode);
            if (preset != nullptr)
            {
                if(this->preset_mode != preset)
                    dirty = true;
                this->preset_mode = preset; // std::string assignment is fine
            }
            if(dirty)
            this->publish_state();
        }

    }
}