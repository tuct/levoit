#include "philips_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips.text_sensor";

void PhilipsTextSensor::setup() {
  // ESP version and model are known at build time; the MCU version comes from the MCU.
  if (this->type_ == TextSensorType::ESP_VERSION)
    this->publish_state(this->parent_->get_version());
  else if (this->type_ == TextSensorType::MODEL)
    this->publish_state(this->parent_->get_model_name());
}

void PhilipsTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Philips Text Sensor", this); }

}  // namespace philips
}  // namespace esphome
