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
    // Observed payload: 01 01 {on} 02 01 {color_temp} 03 02 {LE16_brightness}
    //   tag01: on/off (01=on, 00=off)
    //   tag02: color temperature (uint8, 0x3A=58 is default warm white)
    //   tag03: brightness (LE16, 0–4095 observed range)
    static std::vector<uint8_t> build_nightlight_command(uint8_t on, uint8_t color_temp, uint16_t brightness)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x0B, 0x55};
      std::vector<uint8_t> payload = {
          0x01, 0x01, on,
          0x02, 0x01, color_temp,
          0x03, 0x02, (uint8_t)(brightness & 0xFF), (uint8_t)(brightness >> 8)};
      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

    // CMD=02 0C 55  —  Breathing mode
    // Observed payload: 01 01 01 03 02 {LE16_max} 02 01 {speed} 04 01 {min} 05 01 48
    //   tag01: mode (01=breathing)
    //   tag02: breathing cycle time in seconds (1–10)
    //   tag03: max brightness (LE16)
    //   tag04: min brightness (uint8)
    //   tag05: 0x48 = color (fixed, warm white)
    // Note: tags appear out of numeric order — match observed payload order exactly.
    // tag05 = color temperature, same as nightlight (observed: 0x48, 0x4A, 0x61 in captures)
    static std::vector<uint8_t> build_breathing_command(uint16_t max_brightness, uint8_t min_brightness, uint8_t speed_sec, uint8_t color_temp)
    {
      std::vector<uint8_t> msg_type = {0x02, 0x0C, 0x55};
      std::vector<uint8_t> payload = {
          0x01, 0x01, 0x01,
          0x03, 0x02, (uint8_t)(max_brightness & 0xFF), (uint8_t)(max_brightness >> 8),
          0x02, 0x01, speed_sec,
          0x04, 0x01, min_brightness,
          0x05, 0x01, color_temp};
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
        return build_nightlight_command(0x00, 0x3A, 0x0000);

      case CommandType::setSproutLightNightlight:
      {
        auto *n_bri = self->get_number(NumberType::LED_VALUE);
        auto *n_ct = self->get_number(NumberType::LED_COLOR_TEMP);
        uint16_t brightness = (n_bri != nullptr) ? static_cast<uint16_t>(n_bri->state) : 0x0C80;
        uint8_t color_temp = (n_ct != nullptr) ? static_cast<uint8_t>(n_ct->state) : 0x3A;
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutLightNightlight: brightness=%u color_temp=%u", brightness, color_temp);
        return build_nightlight_command(0x01, color_temp, brightness);
      }

      case CommandType::setSproutLightBreathing:
      {
        auto *n_max = self->get_number(NumberType::LED_VALUE);
        auto *n_min = self->get_number(NumberType::LED_BRIGHTNESS_MIN);
        auto *n_spd = self->get_number(NumberType::LED_SPEED);
        auto *n_ct = self->get_number(NumberType::LED_COLOR_TEMP);
        uint16_t max_bri = (n_max != nullptr) ? static_cast<uint16_t>(n_max->state) : 0x0C80;
        uint8_t min_bri = (n_min != nullptr) ? static_cast<uint8_t>(n_min->state) : 0x07;
        uint8_t speed = (n_spd != nullptr) ? static_cast<uint8_t>(n_spd->state) : 0x05;
        uint8_t color_temp = (n_ct != nullptr) ? static_cast<uint8_t>(n_ct->state) : 0x48;
        ESP_LOGD(TAG_SPROUT_CMD, "setSproutLightBreathing: max=%u min=%u speed=%u ct=%u", max_bri, min_bri, speed, color_temp);
        return build_breathing_command(max_bri, min_bri, speed, color_temp);
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

      // CMD=02 06 55  —  Sprout WiFi LED blink animation
      // PAY=01 02 {LE16_period_ms}  — tag01: blink period in ms (LE16)
      // Original firmware ramps 50→500ms on boot; we use 500ms steady blink for "connecting".
      case CommandType::setWifiLedBlinking:
      {
        std::vector<uint8_t> msg_type = {0x02, 0x06, 0x55};
        std::vector<uint8_t> payload = {0x01, 0x02, 0xF4, 0x01}; // 500ms period
        ESP_LOGD(TAG_SPROUT_CMD, "setWifiLedBlinking (CMD=02 06 55, 500ms)");
        return build_levoit_message(msg_type, payload, messageUpCounter);
      }

      default:
        // Not a Sprout-specific command — caller should fall back to build_vital_command()
        return {};
      }
    }

  } // namespace levoit
} // namespace esphome
