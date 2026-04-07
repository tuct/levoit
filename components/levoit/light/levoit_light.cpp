#include "levoit_light.h"
#include "../levoit.h"
#include "../types.h"
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
  // Range: 2000K (500 mireds, warm) to 3500K (286 mireds, cool)
  traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(286.0f);
  traits.set_max_mireds(500.0f);
  return traits;
}

void LevoitSproutLight::write_state(light::LightState *state) {
  if (publishing_) return;
  if (parent_ == nullptr) return;
  bool is_on = state->current_values.is_on();
  // get_color_temperature() returns mireds directly (same value ESPHome logs)
  float ct_mireds = state->current_values.get_color_temperature();
  float white_brightness = state->current_values.get_brightness();

  // brightness: 0.0–1.0 → 0–100 pct (uint8)
  uint8_t bri_pct = static_cast<uint8_t>(white_brightness * 100.0f + 0.5f);
  if (bri_pct > 100) bri_pct = 100;

  // color temp: mireds → Kelvin, clamped to device range 2000–3500K
  if (ct_mireds < 286.0f) ct_mireds = 286.0f;
  if (ct_mireds > 500.0f) ct_mireds = 500.0f;
  uint16_t ct_k = static_cast<uint16_t>(1000000.0f / ct_mireds + 0.5f);

  if (!is_on) {
    ESP_LOGD(TAG, "LED off");
    parent_->sendCommand(setSproutLedOff);
  } else if (breathing_active_) {
    ESP_LOGD(TAG, "LED breathing: bri_pct=%u ct_k=%u", bri_pct, ct_k);
    parent_->sendSproutLightDirect(true, true, bri_pct, ct_k);
  } else {
    ESP_LOGD(TAG, "LED nightlight: bri_pct=%u ct_k=%u", bri_pct, ct_k);
    parent_->sendSproutLightDirect(true, false, bri_pct, ct_k);
  }
}

void LevoitSproutLight::apply_from_mcu(bool on, float brightness, float color_temp, bool breathing) {
  if (state_ == nullptr) return;
  // Don't override HA's in-flight transition with stale MCU status
  if (state_->is_transformer_active()) return;
  publishing_ = true;
  auto call = state_->make_call();
  call.set_transition_length(0);
  if (!on) {
    call.set_state(false);
  } else {
    call.set_state(true);
    call.set_brightness(brightness < 0.01f ? 0.01f : brightness);
    // CT and effect are write-only: syncing them back from MCU causes feedback loops.
    // Both are exclusively controlled from HA side.
  }
  call.perform();
  publishing_ = false;
}

}  // namespace levoit
}  // namespace esphome
