#pragma once
#include <vector>
#include <cstdint>
#include "types.h"

namespace esphome
{
  namespace levoit
  {
    class Levoit; // forward declaration

    std::vector<uint8_t> build_core_command(Levoit *self, CommandType cmd);

  } // namespace levoit
} // namespace esphome
