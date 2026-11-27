#pragma once
#include "lv600s_protocol.h"

namespace levoit
{
    class LevoitDevice;

    void decode_lv600s_status(LevoitDevice *self, const lv600s::Status &status);
}
