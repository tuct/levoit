#pragma once
#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "../philips.h"

namespace esphome {
namespace philips {

// Sleep timer in hours (900 series only): 0 = off, 1-12 h.
// The minutes actually remaining are a separate read-only sensor
// (SensorType::TIMER_REMAINING) — the MCU derives and counts those down itself.
class PhilipsNumber : public number::Number, public Component {
 public:
  void set_parent(Philips *p) { parent_ = p; }
  void set_type(NumberType t) { type_ = t; }
  void dump_config() override;

 protected:
  void control(float value) override;
  Philips *parent_{nullptr};
  NumberType type_{NumberType::TIMER};
};

}  // namespace philips
}  // namespace esphome
