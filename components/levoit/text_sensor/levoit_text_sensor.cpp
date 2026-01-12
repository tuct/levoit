#include "levoit_text_sensor.h"
#include "../levoit.h"
#include "esphome/core/entity_base.h"



namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.text_sensor";

    void LevoitTextSensor::setup() {

      switch (this->type_) {
        case TextSensorType::MCU_VERSION:
        case TextSensorType::AUTO_MODE_ROOM_SIZE_HIGH_FAN:
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        case TextSensorType::ESP_VERSION:
          this->publish_state(this->parent_->get_version());
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;

        default:
          break;
      }
    }
    void LevoitTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Levoit TextSensor", this); }



  } // namespace levoit
} // namespace esphome
