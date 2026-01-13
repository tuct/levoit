#pragma once
#include <string>
#include <cstdint>

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

  } // namespace levoit
} // namespace esphome
