#include "levoit_button.h"
#include "esphome/core/log.h"
#include "../levoit.h"

namespace esphome
{
    namespace levoit
    {

        static const char *const TAG = "levoit.button";

        void LevoitButton::setup()
        {
            ESP_LOGD(TAG, "Setting up Levoit Button");
        }

        void LevoitButton::dump_config()
        {
            LOG_BUTTON("", "Levoit Button", this);
        }

        void LevoitButton::press_action()
        {
            if (this->parent_ == nullptr)
            {
                ESP_LOGW(TAG, "Button has no parent; ignoring press");
                return;
            }

            switch (this->type_)
            {
            case ButtonType::RESET_FILTER_STATS:
            {
                // Reset CADR usage and total runtime, persist via setters
                this->parent_->set_used_cadr(0.0f);
                this->parent_->set_total_runtime(0);

                // Immediately publish updated filter life percent sensor and low binary sensor
                this->parent_->publish_filter_stats_now();

                ESP_LOGI(TAG, "Reset filter stats: used_cadr=0, total_runtime=0");
                if (this->parent_->has_mcu_filter_reset())
                {
                    // Core200S: the MCU keeps a filter counter of its own that the
                    // stock app resets with 01 E4 A5, so clear that too. It is not
                    // what feeds the sensor - the ESP-side estimate above is.
                    this->parent_->sendCommand(CommandType::resetFilter);
                    ESP_LOGI(TAG, "Sent MCU filter reset");
                }
                break;
            }
            default:
                ESP_LOGW(TAG, "Unhandled button type");
                break;
            }
        }

    } // namespace levoit
} // namespace esphome
