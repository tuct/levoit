#include "levoit_fan.h"
#include "../levoit.h"
#include "../fan_modes.h"
#include "esphome/core/log.h"
#include <set>

namespace esphome
{
    namespace levoit_component
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
            {4, "Turbo"},  // EverestAir top speed, exposed as a fan mode
            {5, "Pet"},
            {6, "Dry"},
        };
        static int preset_to_device_mode(const char *preset)
        {
            if (!preset) return -1;
            for (auto &m : MODE_MAP)
                if (std::strcmp(preset, m.preset) == 0)
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
            ModelType model = parent_->get_model();
            bool isCoreModel = model == ModelType::CORE200S || model == ModelType::CORE300S || model == ModelType::CORE400S || model == ModelType::CORE600S;
            bool isSprout = model == ModelType::SPROUT;
            bool isEverest = model == ModelType::EVERESTAIR;
            bool isEleva = model == ModelType::ELEVA300X;
            auto restore = this->restore_state_();
            if (restore.has_value())
            {
                restore->apply(*this);
            }
            // based on model, set capabilities
            std::vector<const char *> preset_modes;
            for (const auto &m : MODE_MAP)
            {
                if (m.device == 6 && model != ModelType::SUPERIOR6000S)
                    continue; // Dry is Superior 6000S-only
                if (!isEverest && std::strcmp(m.preset, "Turbo") == 0)
                    continue; // Turbo is EverestAir-only (fan mode 4)
                if (isEverest && std::strcmp(m.preset, "Pet") == 0)
                    continue; // EverestAir has no Pet mode
                if ((isCoreModel || model == ModelType::SUPERIOR6000S) && std::strcmp(m.preset, "Pet") == 0)
                    continue; // Core and Superior models do not have Pet mode
                if (isEleva && std::strcmp(m.preset, "Pet") == 0)
                    continue; // Eleva 300X: Manual/Sleep/Auto; turbo is fan level 4
                if (!parent_->device().has_auto() && std::strcmp(m.preset, "Auto") == 0)
                    continue; // Core200S has no Auto mode (no PM2.5 sensor)
                if (isSprout && (std::strcmp(m.preset, "Sleep") == 0 || std::strcmp(m.preset, "Pet") == 0))
                    continue; // Sprout only has Manual and Auto modes
                preset_modes.push_back(m.preset);
            }

            // The speed count is the core's per-model fact (LevoitDevice::speed_count):
            // 3 on the Core 200S / 300S and the EverestAir (its 4th step is the
            // "Turbo" preset, not a fan level), 9 on the Superior 6000S, 4 elsewhere.
            // The entity had its own table with 4 for the Superior, which clamped
            // its speeds 5-9 to 4.
            this->speed_count_ = parent_->device().speed_count();

            // Construct traits
            this->traits_ =
                fan::FanTraits(this->has_oscillating_, this->speed_count_ > 0, this->has_direction_, this->speed_count_);
            this->set_supported_preset_modes(preset_modes);
    
     
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
            esphome::StringRef cur_preset = this->get_preset_mode();
            const auto preset = call.get_preset_mode();

            // ---- power ----
            if (call.get_state().has_value())
            {
                bool new_state = *call.get_state();
                if (parent_ && parent_->get_model() == ModelType::SUPERIOR6000S)
                {
                    // The MCU may keep running to dry the filter after an off
                    // request. Wait for its report and allow explicit retries.
                    power_cmd = new_state ? 1 : 0;
                }
                else if (new_state != cur_state)
                {
                    this->state = new_state;
                    power_cmd = new_state ? 1 : 0;
                }
            }

            // ---- speed ----
            if (call.get_speed().has_value() && (this->speed_count_ > 0))
            {
                int new_speed = *call.get_speed();
                // A speed call issued while a non-Manual preset is active must still
                // be sent even if the level is unchanged: the operating mode changes.
                // Sleep/Auto leave this->speed at its last manual value because the
                // MCU reports no fan level for those modes (apply_device_status gets
                // speed_level == -1), so "same level" is not "no change" here.
                // A reported Superior mode can lack a preset (e.g. Humidity),
                // leaving the entity label at Manual. Use the core mode as well.
                const bool superior_not_manual =
                    parent_ && parent_->get_model() == ModelType::SUPERIOR6000S &&
                    parent_->device().state().fan.mode != 0;
                const bool leaving_preset =
                    preset == nullptr && (superior_not_manual ||
                    (cur_preset != "Manual" && (!cur_preset.empty() ||
                     (parent_ && parent_->get_model() == ModelType::SUPERIOR6000S))));
                if (new_speed != cur_speed || leaving_preset)
                {
                    this->speed = new_speed;
                    speed_cmd = new_speed;
                    if (preset == nullptr && (superior_not_manual || cur_preset.empty() || cur_preset != "Manual"))
                    {
                        this->set_preset_mode_("Manual");
                        mode_cmd = preset_to_device_mode("Manual");
                    }
                }
            }

            // ---- preset/mode ----
            if (preset != nullptr && (cur_preset.empty() || cur_preset != preset))
            {
                // Sync base fan preset state
                this->set_preset_mode_(preset);
                mode_cmd = preset_to_device_mode(preset);
            }

            if (!parent_)
            {
                ESP_LOGW(TAG, "No parent set for fan");
                this->publish_state(); // still reflect local changes if any
                return;
            }

            // Only sends fields != -1 (as long as your parent respects sentinels)
            if (mode_cmd != -1 && ::levoit::fan_operating_mode_to_option(mode_cmd) != nullptr)
            {
                parent_->publish_select(SelectType::FAN_OPERATING_MODE_SELECT, mode_cmd);
            }
            parent_->on_fan_command(power_cmd, speed_cmd, mode_cmd);

            this->publish_state();
        }

        void LevoitFan::apply_device_status(int power, int speed_level, int mode)
        {
            bool dirty = false;
            
            // power: -1 = keep current
            if (power != -1)
            {
                bool new_state = (power == 1);
                if (this->state != new_state)
                {
                    dirty = true;
                    this->state = new_state;
                }
            }

            // speed_level: -1 = keep current
            if (speed_level != -1 && this->speed_count_ > 0)
            {
                if (speed_level < 1)
                    speed_level = 1;
                if (speed_level > this->speed_count_)
                    speed_level = this->speed_count_;

                if (this->speed != speed_level)
                {
                    dirty = true;
                    this->speed = speed_level;
                }
            }

            const char *preset = device_mode_to_preset(mode);
            if (mode != -1 && this->parent_ != nullptr &&
                ::levoit::fan_operating_mode_to_option(mode) != nullptr)
            {
                this->parent_->publish_select(SelectType::FAN_OPERATING_MODE_SELECT, mode);
            }
            if (preset != nullptr)
            {
                ESP_LOGD(TAG, "Device mode %d maps to preset '%s'", mode, preset);
                esphome::StringRef cur = this->get_preset_mode();
                if (cur.empty() || cur != preset)
                {
                    dirty = true;
                    ESP_LOGD(TAG, "Updating preset mode to '%s'", preset);
                    this->set_preset_mode_(preset);
                }
            }
            
            // Only publish state if something changed
            if (dirty)
            {
                ESP_LOGD(TAG, "Updating Fan state ");
                this->publish_state();
            }
        }

    }
}