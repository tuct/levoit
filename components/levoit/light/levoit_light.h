#pragma once
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_effect.h"

namespace esphome {
namespace levoit {

class Levoit;

// Breathing effect: signals LevoitSproutLight that breathing mode is active.
// The MCU handles the actual animation — this just toggles a flag.
class LevoitSproutBreathingEffect : public light::LightEffect {
 public:
  LevoitSproutBreathingEffect(const char *name, bool *active_flag)
      : LightEffect(name), active_flag_(active_flag) {}
  void start() override { *active_flag_ = true; }
  void stop() override { *active_flag_ = false; }
  void apply() override {}

 private:
  bool *active_flag_;
};

class LevoitSproutLight : public light::LightOutput {
 public:
  void set_parent(Levoit *parent) { parent_ = parent; }

  // Called by ESPHome when the LightState is created — attach the breathing effect here
  void setup_state(light::LightState *state) override;

  light::LightTraits get_traits() override;

  // Called by ESPHome whenever HA changes brightness / on-off / effect
  void write_state(light::LightState *state) override;

  // Called from vital_status via Levoit to sync MCU → HA without triggering write_state
  void apply_from_mcu(bool on, float brightness, float color_temp, bool breathing);

 private:
  Levoit *parent_{nullptr};
  light::LightState *state_{nullptr};
  bool breathing_active_{false};
  bool publishing_{false};
};

}  // namespace levoit
}  // namespace esphome
