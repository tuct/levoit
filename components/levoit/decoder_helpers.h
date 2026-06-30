#pragma once
#include <string>
#include <cstdint>
#include <cmath>

namespace esphome
{
  namespace levoit
  {

    // Format minutes into human-readable string (e.g., 502 -> "8h 22min").
    static inline std::string format_duration_minutes(uint16_t minutes)
    {
      if (minutes == 0)
        return "Off";

      const uint16_t hours = minutes / 60;
      const uint16_t mins = minutes % 60;

      if (hours > 0)
        return std::to_string(hours) + "h " + std::to_string(mins) + " min";

      return std::to_string(mins) + " min";
    }

    static inline std::string format_duration_seconds(uint16_t seconds)
    {
      if (seconds == 0)
        return "Off";

      const uint16_t mins = seconds / 60;
      const uint16_t secs = seconds % 60;

      if (mins > 0)
        return std::to_string(mins) + " min " + std::to_string(secs) + "s";

      return std::to_string(secs) + "s";
    }

    static inline float core_room_size_factor()
    {
      return 10.764f * 3.15f;
    }

    static inline uint32_t encode_core_room_size_raw(float m2)
    {
      return static_cast<uint32_t>(std::round(m2 * core_room_size_factor()));
    }

    static inline float decode_core_room_size_m2(uint16_t raw)
    {
      return std::round(static_cast<float>(raw) / core_room_size_factor());
    }

  } // namespace levoit
} // namespace esphome
