#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "types.h"

namespace esphome {
namespace levoit {

struct LevoitTLV {
  uint8_t tag{0};
  uint8_t value_len{0};
  const uint8_t *value_ptr{nullptr};
  uint32_t value_u32{0};
};

bool extract_tlvs_from_payload(ModelType model,
                               const uint8_t *payload,
                               size_t payload_len,
                               std::vector<LevoitTLV> &out,
                               size_t start_offset = 0);

}  // namespace levoit
}  // namespace esphome
