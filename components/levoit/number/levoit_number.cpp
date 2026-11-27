#include "levoit_number.h"
#include "../levoit.h"
#include "esphome/core/log.h"
#include "esphome/components/number/number_traits.h"
#include <algorithm>
#include <cmath>

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.number";

    float LevoitNumber::normalize_room_size(float value) const {
      if (parent_ && parent_->get_model() == ModelType::VITAL100S &&
          this->traits.get_step() == 1.0f &&
          (this->type_ == NumberType::EFFICIENCY_ROOM_SIZE ||
           this->type_ == NumberType::AUTO_PROFILE_ROOM_SIZE_INPUT)) {
        return std::round(value);
      }
      return value;
    }

    void LevoitNumber::setup() {
      // pick default
      this->traits.set_mode(number::NumberMode::NUMBER_MODE_AUTO);

      switch (this->type_) {
        case NumberType::EFFICIENCY_ROOM_SIZE:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);
          break;

        case NumberType::AUTO_PROFILE_ROOM_SIZE_INPUT:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_BOX);

          pref_ = global_preferences->make_preference<float>(fnv1_hash("levoit_room_size_input"));
          {
            float saved;
            if (pref_.load(&saved)) {
              saved = this->normalize_room_size(saved);
              float clamped = std::max(this->traits.get_min_value(),
                                       std::min(this->traits.get_max_value(), saved));
              ESP_LOGI(TAG, "Restored auto profile room size input: %.0f m²", clamped);
              this->publish_state(clamped);
              if (parent_) parent_->note_number_state(this->type_, clamped);
            } else {
              ESP_LOGI(TAG, "No saved room size input, defaulting to %.0f m²",
                       this->traits.get_min_value());
              this->publish_state(this->traits.get_min_value());
              if (parent_) parent_->note_number_state(this->type_, this->traits.get_min_value());
            }
          }
          break;

        case NumberType::TIMER:
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);
          break;

        case NumberType::VENT_ANGLE:
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
            if (parent_) parent_->note_number_state(this->type_, saved_value);
          } else {
            ESP_LOGI(TAG, "No saved filter lifetime, using default: 6 months");
            this->publish_state(6.0);
            if (parent_) parent_->note_number_state(this->type_, 6.0f);
          }
          break;

        default:
          break;
      }
    }
    void LevoitNumber::dump_config() { LOG_NUMBER("", "Levoit Number", this); }

    void LevoitNumber::control(float value)
    {
      value = this->normalize_room_size(value);
      // Whole-number entities are truncated; the Eleva quiet window is in
      // fractional hours (21.5 = 21:30) and must keep its fraction.
      const bool fractional = this->type_ == NumberType::QUIET_START || this->type_ == NumberType::QUIET_END;
      float state = fractional ? value : static_cast<float>(static_cast<uint32_t>(value));
      // Optimistic update for HA UI
      this->publish_state(state);
      
      // Save to preferences if filter_lifetime_months or auto_profile_room_size_input
      if (this->type_ == NumberType::FILTER_LIFETIME_MONTHS) {
        pref_.save(&value);
        ESP_LOGD(TAG, "Saved filter lifetime: %.0f months", value);
      }
      if (this->type_ == NumberType::AUTO_PROFILE_ROOM_SIZE_INPUT) {
        pref_.save(&value);
        ESP_LOGD(TAG, "Saved auto profile room size input: %.0f m²", value);
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
