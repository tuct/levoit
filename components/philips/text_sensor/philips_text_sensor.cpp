#include "philips_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips.text_sensor";

void PhilipsTextSensor::dump_config() { LOG_TEXT_SENSOR("", "Philips Text Sensor", this); }

}  // namespace philips
}  // namespace esphome
