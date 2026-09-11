#include "philips_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips.number";

void PhilipsNumber::dump_config() { LOG_NUMBER("", "Philips Number", this); }

void PhilipsNumber::control(float value) {
  if (this->parent_ == nullptr) return;
  if (this->type_ == NumberType::TIMER) {
    if (value < 0) value = 0;
    this->parent_->set_timer_hours((uint8_t) value);
  }
  // The MCU echoes an unsolicited status ~25 ms after every SET, so the state
  // published from there is authoritative. Publish now for a responsive UI.
  this->publish_state(value);
}

}  // namespace philips
}  // namespace esphome
