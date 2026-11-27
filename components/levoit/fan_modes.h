#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome {
namespace levoit {

// Fan operating mode: the MCU's raw value (TLV 0x03, and the Vital daytime
// preset TLV 0x22) and its select option. Raw 3 (humidity) is not used by
// any purifier. One table for the decoder (raw -> option) and the select
// (option -> raw).
struct FanOperatingMode {
  uint8_t raw;
  const char *option;
};

inline constexpr FanOperatingMode FAN_OPERATING_MODES[] = {
    {0, "Manual"}, {1, "Sleep"}, {2, "Auto"}, {4, "Turbo"}, {5, "Pet"},
};

// The select option for a raw mode; nullptr for an unknown one.
inline const char *fan_operating_mode_to_option(uint32_t raw) {
  for (const auto &mode : FAN_OPERATING_MODES) {
    if (mode.raw == raw)
      return mode.option;
  }
  return nullptr;
}

// The raw mode for a select option; -1 for an unknown one. Plain strings, so
// this header builds without ESPHome (docs/shared-core.md).
inline int fan_operating_mode_to_raw(const char *option, size_t len) {
  for (const auto &mode : FAN_OPERATING_MODES) {
    if (std::strlen(mode.option) == len && std::memcmp(mode.option, option, len) == 0)
      return mode.raw;
  }
  return -1;
}
inline int fan_operating_mode_to_raw(const std::string &option) {
  return fan_operating_mode_to_raw(option.data(), option.size());
}

}  // namespace levoit
}  // namespace esphome
