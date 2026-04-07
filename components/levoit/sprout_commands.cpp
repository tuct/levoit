#include "sprout_commands.h"
#include "levoit_message.h"
#include "levoit.h"
#include "number/levoit_number.h"
#include "select/levoit_select.h"
#include "esphome/core/log.h"
#include <algorithm>

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG_SPROUT_CMD = "levoit.sprout_cmd";

    // CMD=02 0B 55  —  Nightlight mode
    // Observed payload: 01 01 {on} 02 01 {brightness_pct} 03 02 {LE16_color_temp_k}
    //   tag01: on/off (01=on, 00=off)
    //   tag02: brightness 0–100 (uint8 percentage)
    //   tag03: color temperature in Kelvin (LE16, e.g. 0x07D0=2000K, 0x0DAC=3500K)
    static std::vector<uint8_t> build_nightlight_command(uint8_t on, uint8_t brightness_pct, uint16_t color_temp_k)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x0B, 0x55};
      std::vector<uint8_t> payload = {
          0x01, 0x01, on,
          0x02, 0x01, brightness_pct,
          0x03, 0x02, (uint8_t)(color_temp_k & 0xFF), (uint8_t)(color_temp_k >> 8)};
      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

    // CMD=02 0C 55  —  Breathing mode
    // Confirmed payload order (tags appear out of numeric order):
    //   tag01: 0x01 = breathing on
    //   tag03: color temperature Kelvin LE16 (e.g. 0x0DAC=3500K)
    //   tag02: breathing cycle time in seconds (1–10)
    //   tag04: min brightness 0–100 (uint8 percentage)
    //   tag05: max brightness 0–100 (uint8 percentage)
    static std::vector<uint8_t> build_breathing_command(uint8_t max_brightness_pct, uint8_t min_brightness_pct, uint8_t speed_sec, uint16_t color_temp_k)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x0C, 0x55};
      std::vector<uint8_t> payload = {
          0x01, 0x01, 0x01,
          0x03, 0x02, (uint8_t)(color_temp_k & 0xFF), (uint8_t)(color_temp_k >> 8),
          0x02, 0x01, speed_sec,
          0x04, 0x01, min_brightness_pct,
          0x05, 0x01, max_brightness_pct};
      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

    // CMD=02 07 55  —  White noise on/off
    // Observed payload: 01 03 {active} {volume} {sound}  03 03 05 08 F2
    //   tag01: 3-byte state: active(0/1), volume(0-255), sound_index(0-14)
    //   tag03: constant trailer {05 08 F2} — purpose unknown, always present
    static std::vector<uint8_t> build_white_noise_command(uint8_t active, uint8_t volume, uint8_t sound_index)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x07, 0x55};
      std::vector<uint8_t> payload = {
          0x01, 0x03, active, volume, sound_index,
          0x03, 0x03, 0x05, 0x08, 0xF2};
      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

    // CMD=02 02 55  —  White noise fan mode enable/disable
    // Observed payload: 10 01 01 (enable) / 10 01 00 (disable)
    static std::vector<uint8_t> build_white_noise_mode_command(bool enable)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x02, 0x55};
      std::vector<uint8_t> payload = {0x10, 0x01, static_cast<uint8_t>(enable ? 0x01 : 0x00)};
      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

    std::vector<uint8_t> build_sprout_command(Levoit *self, CommandType cmd)
    {
      switch (cmd)
      {
      case CommandType::setSproutLedOff:
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutLedOff");
        return build_nightlight_command(0x00, 0x00, 0x0000);

      case CommandType::setSproutLightNightlight:
      {
        uint8_t bri_pct = self->get_pending_led_bri();
        uint16_t ct_k = self->get_pending_led_ct();
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutLightNightlight: bri_pct=%u ct_k=%u", bri_pct, ct_k);
        return build_nightlight_command(0x01, bri_pct, ct_k);
      }

      case CommandType::setSproutLightBreathing:
      {
        uint8_t max_pct = self->get_pending_led_bri();
        uint16_t ct_k = self->get_pending_led_ct();
        auto *n_min = self->get_number(NumberType::LED_BRIGHTNESS_MIN);
        auto *n_spd = self->get_number(NumberType::LED_SPEED);
        uint8_t min_pct = (n_min != nullptr) ? static_cast<uint8_t>(n_min->state) : 5;
        uint8_t speed = (n_spd != nullptr) ? static_cast<uint8_t>(n_spd->state) : 5;
        // Clamp speed to minimum 1
        if (speed < 1) speed = 1;
        // If min >= max, pull min down to max-20 (floor 0)
        if (min_pct >= max_pct)
          min_pct = (max_pct >= 20) ? max_pct - 20 : 0;
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutLightBreathing: max_pct=%u min_pct=%u speed=%u ct_k=%u", max_pct, min_pct, speed, ct_k);
        return build_breathing_command(max_pct, min_pct, speed, ct_k);
      }

      case CommandType::setSproutWhiteNoiseOn:
      case CommandType::setSproutWhiteNoiseOff:
      {
        bool active = (cmd == CommandType::setSproutWhiteNoiseOn);
        auto *n_vol = self->get_number(NumberType::WHITE_NOISE_VOLUME);
        uint8_t volume = (n_vol != nullptr) ? static_cast<uint8_t>(n_vol->state) : 128;

        // Get current sound index from select state
        uint8_t sound_index = 0;
        auto *s_sound = self->get_select(SelectType::WHITE_NOISE_SOUND);
        if (s_sound != nullptr && s_sound->has_state()) {
          const auto &opts = s_sound->traits.get_options();
          auto it = std::find(opts.begin(), opts.end(), s_sound->current_option());
          if (it != opts.end())
            sound_index = static_cast<uint8_t>(it - opts.begin());
        }

        ESP_LOGD(TAG_SPROUT_CMD, "%s: vol=%u sound=%u",
                 active ? "setSproutWhiteNoiseOn" : "setSproutWhiteNoiseOff",
                 volume, sound_index);
        return build_white_noise_command(active ? 0x01 : 0x00, volume, sound_index);
      }

      case CommandType::setSproutWhiteNoiseModeOn:
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutWhiteNoiseModeOn");
        return build_white_noise_mode_command(true);

      case CommandType::setSproutWhiteNoiseModeOff:
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutWhiteNoiseModeOff");
        return build_white_noise_mode_command(false);

      // CMD=02 06 55  —  Sprout AQI scale: sets the AQI max value shown on device display (0–500)
      // PAY=01 02 {LE16_value}  — tag01: LE16 AQI value (e.g. 0x0037=55, 0x01F4=500)
      case CommandType::setSproutAqiScale:
      {
        uint16_t aqi = self->get_pending_aqi();
        std::vector<uint8_t> msg_type = {0x02, 0x06, 0x55};
        std::vector<uint8_t> payload = {0x01, 0x02, (uint8_t)(aqi & 0xFF), (uint8_t)(aqi >> 8)};
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutAqi: aqi=%u", aqi);
        return build_levoit_message(msg_type, payload, messageUpCounter);
      }

      default:
        // Not a Sprout-specific command — caller should fall back to build_vital_command()
        return {};
      }
    }

  } // namespace levoit
} // namespace esphome
