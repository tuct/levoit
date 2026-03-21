#include "levoit_number.h"
#include "../levoit.h"
#include "esphome/core/log.h"
#include "esphome/components/number/number_traits.h"

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.number";

    void LevoitNumber::setup() {
      // pick default
      this->traits.set_mode(number::NumberMode::NUMBER_MODE_AUTO);

      switch (this->type_) {
        case NumberType::EFFICIENCY_ROOM_SIZE:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);
          break;

        case NumberType::TIMER:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);
          break;

        case NumberType::FILTER_LIFETIME_MONTHS:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);

          // Setup preferences for persistent storage
          pref_ = global_preferences->make_preference<float>(fnv1_hash("levoit_filter_months"));

          // Restore saved value or use default
          float saved_value;
          if (pref_.load(&saved_value)) {
            ESP_LOGI(TAG, "Restored filter lifetime: %.0f months", saved_value);
            this->publish_state(saved_value);
          } else {
            ESP_LOGI(TAG, "No saved filter lifetime, using default: 6 months");
            this->publish_state(6.0);
          }
          break;

        default:
          break;
      }
    }
    void LevoitNumber::dump_config() { LOG_NUMBER("", "Levoit Number", this); }

    void LevoitNumber::control(float value)
    {
      uint32_t state = static_cast<uint32_t>(value);
      // Optimistic update for HA UI
      this->publish_state(state);
      
      // Save to preferences if filter_lifetime_months
      if (this->type_ == NumberType::FILTER_LIFETIME_MONTHS) {
        pref_.save(&value);
        ESP_LOGD(TAG, "Saved filter lifetime: %.0f months", value);
      }
      
      if (!parent_)
      {
        ESP_LOGW(TAG, "No parent set for number");
        return;
      }

      parent_->on_number_command(this->type_, state);
    }

  } // namespace levoit
} // namespace esphome
