#pragma once
#include <cstddef>
#include <cstdint>
#include "levoit.h"
#include "types.h"

namespace esphome {
namespace levoit {

// Decode async MCU event notifications (CMD=02 08 55, MCU→ESP).
// Called from dispatcher when SPROUT receives ptype0=0x08, ptype1=0x55.
void decode_sprout_event(Levoit *self,
                         const uint8_t *payload,
                         size_t payload_len);

}  // namespace levoit
}  // namespace esphome
