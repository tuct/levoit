#include "philips_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips.text_sensor";

void PhilipsTextSensor::setup() {
  // The ESP version is known at build time; the MCU version comes from the MCU.
  if (this->type_ == TextSensorType::ESP_VERSION)
    this->publish_state(this->parent_->get_version());
}

void PhilipsTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Philips Text Sensor", this); }

}  // namespace philips
}  // namespace esphome
