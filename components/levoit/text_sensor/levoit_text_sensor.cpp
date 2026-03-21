#include "levoit_text_sensor.h"
#include "../levoit.h"



namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.text_sensor";

    void LevoitTextSensor::setup() {
      switch (this->type_) {
        case TextSensorType::ESP_VERSION:
          this->publish_state(this->parent_->get_version());
          break;
        default:
          break;
      }
    }
    void LevoitTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Levoit TextSensor", this); }



  } // namespace levoit
} // namespace esphome
