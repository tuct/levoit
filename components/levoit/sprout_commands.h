#pragma once
#include <vector>
#include <cstdint>
#include "types.h"

namespace esphome
{
  namespace levoit
  {
    class Levoit; // forward declaration

    // Build a Sprout-specific command packet.
    // Returns empty vector if cmd is not a Sprout command — caller should fall back to build_vital_command().
    std::vector<uint8_t> build_sprout_command(Levoit *self, CommandType cmd);

  } // namespace levoit
} // namespace esphome
