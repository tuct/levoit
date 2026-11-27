// The frame layer of components/levoit built without ESPHome, by a plain C++
// compiler (.github/workflows/host-tests.yml). What it proves: these files
// depend on nothing but the standard library, so a Matter firmware can share
// them. See docs/shared-core.md.
//
//   g++ -std=c++17 -Wall -Wextra tests/host/levoit_frame_test.cpp \
//     components/levoit/tlv.cpp components/levoit/levoit_message.cpp \
//     -o levoit-frame-test && ./levoit-frame-test
//
// The component folder is deliberately not on the include path (no -I), as in
// an ESPHome build: its VERSION file would shadow the standard <version>
// header on a case-insensitive file system.

#include <cstdio>
#include <string>
#include <vector>

#include "../../components/levoit/decoder_helpers.h"
#include "../../components/levoit/fan_modes.h"
#include "../../components/levoit/levoit_message.h"
#include "../../components/levoit/tlv.h"
#include "../../components/levoit/types.h"

using namespace levoit;

static int failures = 0;

#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                        \
    }                                                                    \
  } while (0)

// A frame captured on a Vital 100S, docs/MCU_2.0.0_baseline.md:
// fan.set_preset_mode Sleep, counter 0x4D.
static void test_build_message() {
  const std::vector<uint8_t> want = {0xA5, 0x22, 0x4D, 0x07, 0x00, 0x88, 0x02,
                                     0x02, 0x55, 0x00, 0x01, 0x01, 0x01};
  auto got = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x01}, 0x4D);
  CHECK(got == want);
  CHECK(levoit_checksum(want.data(), want.size()) == 0x88);
  // the length byte counts the bytes after the checksum
  CHECK(got[LEVOIT_IDX_LENGTH] == want.size() - (LEVOIT_IDX_CHECKSUM + 1));
}

// select.select_option Quiet on auto_mode, same capture: two TLVs after a count
// byte. Tag 01 (1 byte) = 1, tag 03 (2 bytes) = 0.
static void test_tlv() {
  const uint8_t payload[] = {0x02, 0x01, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00};
  std::vector<LevoitTLV> tlvs;
  CHECK(extract_tlvs_from_payload(ModelType::VITAL100S, payload, sizeof(payload), tlvs, 1));
  CHECK(tlvs.size() == 2);
  if (tlvs.size() == 2) {
    CHECK(tlvs[0].tag == 0x01 && tlvs[0].value_len == 1 && tlvs[0].value_u32 == 1);
    CHECK(tlvs[1].tag == 0x03 && tlvs[1].value_len == 2 && tlvs[1].value_u32 == 0);
  }
  // little-endian values, and a truncated entry fails
  const uint8_t le[] = {0x05, 0x04, 0x78, 0x56, 0x34, 0x12};
  CHECK(extract_tlvs_from_payload(ModelType::VITAL100S, le, sizeof(le), tlvs));
  CHECK(tlvs.size() == 1 && tlvs[0].value_u32 == 0x12345678);
  CHECK(!extract_tlvs_from_payload(ModelType::VITAL100S, le, sizeof(le) - 1, tlvs));
}

static void test_helpers() {
  CHECK(format_duration_minutes(502) == "8h 22 min");
  CHECK(format_duration_minutes(45) == "45 min");
  CHECK(format_duration_minutes(0) == "Off");
  CHECK(format_duration_seconds(90) == "1 min 30s");
  CHECK(decode_core_room_size_m2(encode_core_room_size_raw(40)) == 40);
}

static void test_fan_modes() {
  CHECK(std::string(fan_operating_mode_to_option(2)) == "Auto");
  CHECK(fan_operating_mode_to_option(3) == nullptr);
  CHECK(fan_operating_mode_to_raw("Sleep", 5) == 1);
  CHECK(fan_operating_mode_to_raw(std::string("Pet")) == 5);
  CHECK(fan_operating_mode_to_raw(std::string("Nope")) == -1);
  CHECK(fan_operating_mode_to_raw("", 0) == -1);
}

int main() {
  test_build_message();
  test_tlv();
  test_helpers();
  test_fan_modes();
  if (failures == 0)
    std::printf("levoit frame layer: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
