#pragma once
#include <cstddef>
#include <cstdint>
#include "levoit.h"
#include "types.h"

namespace esphome {
namespace levoit {

void decode_core_status(Levoit *self,
                         ModelType model,
                         const uint8_t *payload,
                         size_t payload_len);
void decode_core_timer(Levoit *self,
                         ModelType model,
                         const uint8_t *payload,
                         size_t payload_len);

}  // namespace levoit
}  // namespace esphome
