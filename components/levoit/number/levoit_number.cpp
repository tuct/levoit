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
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
          this->traits.set_device_class("area");
          this->traits.set_unit_of_measurement("m²");
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);
          this->traits.set_min_value(132);
          this->traits.set_max_value(792);
          this->traits.set_step(14);  // 1 m
      
          break;

        case NumberType::TIMER:
          this->traits.set_device_class("duration");
          this->traits.set_unit_of_measurement("min");
          this->traits.set_mode(number::NumberMode::NUMBER_MODE_SLIDER);  // if you want
          this->traits.set_min_value(0);
          this->traits.set_max_value(720); // 12 hours
          this->traits.set_step(30);
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
      
      if (!parent_)
      {
        ESP_LOGW(TAG, "No parent set for number");
        return;
      }

      parent_->on_number_command(this->type_, state);
    }

  } // namespace levoit
} // namespace esphome
