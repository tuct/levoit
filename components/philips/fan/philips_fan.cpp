#include "philips_fan.h"
#include "../philips.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace philips {

static const char *const TAG = "philips.fan";

// speed 1/2/3 ↔ fan mode (group 0x03, DP 0x0C). Medium is 0x01 on the 600
// series but 0x13 on the 900, so both directions need the model.
static uint8_t speed_to_mode(int speed, uint8_t medium) {
  switch (speed) {
    case 1: return Philips::MODE_SLEEP;
    case 3: return Philips::MODE_TURBO;
    default: return medium;
  }
}
static int mode_to_speed(uint8_t mode, uint8_t medium) {
  if (mode == medium) return 2;
  switch (mode) {
    case Philips::MODE_SLEEP: return 1;
    case Philips::MODE_TURBO: return 3;
    default: return 0;
  }
}

void PhilipsFan::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) restore->apply(*this);
  this->traits_ = fan::FanTraits(false, true, false, 3);  // 3 speeds
  // The sensor-equipped models (AC0651/AC0951) add an Auto preset (fan mode
  // 0x00). Set on the entity (modern API); get_traits() wires it into traits_
  // via wire_preset_modes_().
  if (this->parent_ != nullptr && this->parent_->has_pm_sensor())
    this->set_supported_preset_modes({"Auto"});
}

void PhilipsFan::dump_config() { LOG_FAN("", "Philips Fan", this); }

void PhilipsFan::control(const fan::FanCall &call) {
  if (this->parent_ == nullptr) return;

  if (call.get_state().has_value()) {
    this->state = *call.get_state();
    this->parent_->set_power(this->state);
  }

  const char *preset = call.get_preset_mode();
  if (preset != nullptr) {
    this->set_preset_mode_(preset);
    if (std::strcmp(preset, "Auto") == 0) this->parent_->set_fan_mode(Philips::MODE_AUTO);
  } else if (call.get_speed().has_value()) {
    this->speed = *call.get_speed();
    this->set_preset_mode_("");  // a manual speed leaves Auto
    this->parent_->set_fan_mode(speed_to_mode(this->speed, this->parent_->mode_medium()));
  }
  this->publish_state();
}

void PhilipsFan::apply_state(bool power, uint8_t mode) {
  bool dirty = false;
  if (this->state != power) {
    this->state = power;
    dirty = true;
  }
  if (mode != 0xFF) {
    if (mode == Philips::MODE_AUTO) {
      if (this->get_preset_mode() != "Auto") {
        this->set_preset_mode_("Auto");
        dirty = true;
      }
    } else {
      int sp = mode_to_speed(mode, this->parent_ == nullptr ? Philips::MODE_MEDIUM_600
                                                           : this->parent_->mode_medium());
      if (sp > 0) {
        if (this->speed != sp) {
          this->speed = sp;
          dirty = true;
        }
        if (!this->get_preset_mode().empty()) {
          this->set_preset_mode_("");
          dirty = true;
        }
      }
    }
  }
  if (dirty) this->publish_state();
}

}  // namespace philips
}  // namespace esphome
