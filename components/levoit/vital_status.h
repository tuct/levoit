#pragma once
#include <cstddef>
#include <cstdint>
#include "levoit_device.h"
#include "types.h"

namespace levoit {

void decode_vital_status(LevoitDevice *self,
                         ModelType model,
                         const uint8_t *payload,
                         size_t payload_len);
void decode_vital_timer(LevoitDevice *self,
                         ModelType model,
                         const uint8_t *payload,
                         size_t payload_len);

}  // namespace levoit
