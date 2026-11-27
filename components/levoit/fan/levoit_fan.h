#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace levoit_component {

class Levoit;

class LevoitFan final : public Component, public fan::Fan {
 public:
  LevoitFan() {}
  void setup() override;
  void dump_config() override;
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_speed_count(int count) { this->speed_count_ = count; }
  void set_preset_modes(std::initializer_list<const char *> presets) { this->set_supported_preset_modes(presets); }
  fan::FanTraits get_traits() override {
    this->wire_preset_modes_(this->traits_);
    return this->traits_;
  }

  void apply_device_status(int power, int speed_level, int mode);

 protected:
  void control(const fan::FanCall &call) override;

  bool has_oscillating_{false};
  bool has_direction_{false};
  int speed_count_{0};
  fan::FanTraits traits_;
  Levoit *parent_{nullptr};
};

}
}  