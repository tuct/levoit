#include "levoit_text_sensor.h"
#include "../levoit.h"



namespace esphome
{
  namespace levoit_component
  {

    static const char *const TAG = "levoit.text_sensor";

    void LevoitTextSensor::setup() {
      switch (this->type_) {
        case TextSensorType::ESP_VERSION:
          this->publish_state(this->parent_->get_version());
          break;
        case TextSensorType::MODEL:
          // Known at build time, like the ESP version
          this->publish_state(this->parent_->get_model_name());
          break;
        default:
          break;
      }
    }
    void LevoitTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Levoit TextSensor", this); }



  } // namespace levoit_component
} // namespace esphome
