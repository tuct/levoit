#pragma once
#include <cstdint>
#include "esphome/core/string_ref.h"

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

// The raw mode for a select option; -1 for an unknown one.
inline int fan_operating_mode_to_raw(StringRef option) {
  for (const auto &mode : FAN_OPERATING_MODES) {
    if (option == mode.option)
      return mode.raw;
  }
  return -1;
}

}  // namespace levoit
}  // namespace esphome
