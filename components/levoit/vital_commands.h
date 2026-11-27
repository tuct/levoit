#pragma once
#include <vector>
#include <cstdint>
#include "types.h"

namespace levoit
{
    class LevoitDevice; // forward declaration

    std::vector<uint8_t> build_vital_command(LevoitDevice *self, CommandType cmd);

} // namespace levoit
