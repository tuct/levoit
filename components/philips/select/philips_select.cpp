#include "philips_select.h"
#include "esphome/core/log.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips.select";

void PhilipsSelect::dump_config() { LOG_SELECT("", "Philips Select", this); }

void PhilipsSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) return;
  if (this->type_ == SelectType::DISPLAY_BRIGHTNESS) {
    uint8_t raw = Philips::BRIGHTNESS_OFF;
    if (value == "low") raw = Philips::BRIGHTNESS_LOW;
    else if (value == "bright") raw = Philips::BRIGHTNESS_BRIGHT;
    this->parent_->set_display_brightness(raw);
  }
  // The MCU echoes an unsolicited status ~25 ms after every SET, so the state
  // published from there is authoritative. Publish now for a responsive UI.
  this->publish_state(value);
}

}  // namespace philips
}  // namespace esphome
