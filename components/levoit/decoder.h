#pragma once
#include <cstddef>
#include <cstdint>
#include "levoit_device.h"
#include "types.h"

namespace esphome {
namespace levoit {

void dispatch_decoder(LevoitDevice *self,
                      ModelType model,
                      uint8_t msg_type,
                      uint8_t seq,
                      uint8_t cmd_size_code,
                      uint8_t ptype0,
                      uint8_t ptype1,
                      const uint8_t *payload,
                      size_t payload_len);

}  // namespace levoit
}  // namespace esphome
