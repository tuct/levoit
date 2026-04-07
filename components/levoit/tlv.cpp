
#include "tlv.h"
#include "esphome/core/log.h"

namespace esphome {
namespace levoit {

static uint32_t read_le_u32_(const uint8_t *p, uint8_t n) {
  uint32_t v = 0;
  for (uint8_t i = 0; i < n; i++)
    v |= (uint32_t)p[i] << (8 * i);
  return v;
}

static uint8_t tlv_value_len_from_code_(ModelType model, uint8_t len_code) {
  if (len_code == 0x01) return 1;
  if (len_code == 0x02) return 2;
  if (len_code == 0x04) return 4;
  return len_code;
}

bool extract_tlvs_from_payload(ModelType model,
                               const uint8_t *payload,
                               size_t payload_len,
                               std::vector<LevoitTLV> &out,
                               size_t start_offset) {
  out.clear();
  size_t i = start_offset;

  while (i + 2 <= payload_len) {
    uint8_t tag = payload[i];
    uint8_t len_code = payload[i + 1];
    uint8_t vlen = tlv_value_len_from_code_(model, len_code);

    if (vlen == 0 || i + 2 + vlen > payload_len)
      return false;
    if (vlen > 8)  // skip unreasonably large entries
      return false;

    const uint8_t *val_ptr = payload + i + 2;
    uint32_t val = read_le_u32_(val_ptr, vlen);
    LevoitTLV t;
    t.tag = tag;
    t.value_len = vlen;
    t.value_ptr = val_ptr;
    t.value_u32 = val;
    out.push_back(t);
    i += 2 + vlen;
  }
  return true;
}

} // namespace levoit
} // namespace esphome
