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
          this->set_icon("mdi:chip");
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        case TextSensorType::ESP_VERSION:
          this->set_icon("mdi:chip");
          this->publish_state(this->parent_->get_version());
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        case TextSensorType::TIMER_DURATION_INITIAL:
          this->set_icon("mdi:timer");
          break;  
        case TextSensorType::AUTO_MODE_ROOM_SIZE_HIGH_FAN:
        case TextSensorType::TIMER_DURATION_CURRENT:
          this->set_icon("mdi:progress-clock");
          break;    
        case TextSensorType::ERROR_MESSAGE:
          this->set_icon("mdi:alert-circle-outline");
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        default:
          break;
      }
    }
    void LevoitTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Levoit TextSensor", this); }



  } // namespace levoit
} // namespace esphome
