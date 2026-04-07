#include "levoit_light.h"
#include "../levoit.h"
#include "../types.h"
#include "../number/levoit_number.h"
#include "esphome/core/log.h"

namespace esphome {
namespace levoit {

static const char *const TAG = "levoit.light";

void LevoitSproutLight::setup_state(light::LightState *state) {
  state_ = state;
  state->add_effects({new LevoitSproutBreathingEffect("Breathing", &breathing_active_)});
}

light::LightTraits LevoitSproutLight::get_traits() {
  auto traits = light::LightTraits();
  // COLOR_TEMPERATURE mode gives HA both a brightness slider and a color temp slider.
  // Raw MCU CT is 0–255; we expose it directly as mired value (min=1, max=255).
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(1.0f);
  traits.set_max_mireds(255.0f);
  return traits;
}

void LevoitSproutLight::write_state(light::LightState *state) {
  if (publishing_) return;
  if (parent_ == nullptr) return;

  bool is_on = state->current_values.is_on();
  float brightness = state->current_values.get_brightness();
  // get_color_temperature() returns the mired value; we use min=1 max=255 so it's the raw CT
  float ct_f = state->current_values.get_color_temperature();
  uint8_t ct_raw = static_cast<uint8_t>(ct_f < 1.0f ? 1u : (ct_f > 255.0f ? 255u : (uint8_t)ct_f));

  // Map HA brightness (0.0–1.0) → MCU range (0–4095)
  uint16_t bri_raw = static_cast<uint16_t>(brightness * 4095.0f + 0.5f);

  // Keep companion numbers in sync so build_sprout_command can read them
  auto *n_bri = parent_->get_number(NumberType::LED_VALUE);
  if (n_bri != nullptr)
    n_bri->publish_state(bri_raw);
  auto *n_ct = parent_->get_number(NumberType::LED_COLOR_TEMP);
  if (n_ct != nullptr)
    n_ct->publish_state(ct_raw);

  if (!is_on) {
    ESP_LOGD(TAG, "LED off");
    parent_->sendCommand(setSproutLedOff);
  } else if (breathing_active_) {
    ESP_LOGD(TAG, "LED breathing: max_bri=%u", bri_raw);
    parent_->sendCommand(setSproutLightBreathing);
  } else {
    ESP_LOGD(TAG, "LED nightlight: bri=%u", bri_raw);
    parent_->sendCommand(setSproutLightNightlight);
  }
}

void LevoitSproutLight::apply_from_mcu(bool on, float brightness, float color_temp, bool breathing) {
  if (state_ == nullptr) return;
  publishing_ = true;
  auto call = state_->make_call();
  call.set_transition_length(0);
  if (!on) {
    call.set_state(false);
  } else {
    call.set_state(true);
    call.set_brightness(brightness < 0.01f ? 0.01f : brightness);
    call.set_color_temperature(color_temp);
    call.set_effect(breathing ? "Breathing" : "None");
  }
  call.perform();
  publishing_ = false;
}

}  // namespace levoit
}  // namespace esphome
