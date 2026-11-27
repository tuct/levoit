#pragma once
// Host side of the MCU in-application-programming (DFU) protocol.
//
// The purifier's second MCU (SinOne SC95F8616, 8051 core, 16 KB flash) cannot
// be reached from the ESP32 except over the 0xA5 UART.  In the stock firmware
// the ESP acts as the DFU server: it sends 0x8030 (version + size), the MCU
// answers with its status and preferred MTU, then the MCU PULLS chunks with
// 0x8031 (offset), the ESP answers each with is_last/offset/len/data, and the
// MCU reports verified (0x8032) and burned (0x8033); 0xD101 reboots it.
//
// This class is the same state machine, with the image supplied by a callback
// so the same code works from a const array, a flash partition or a file.
// See docs/mcu-dfu.md for the wire captures.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "levoit_platform.h"

namespace levoit {

class McuDfu {
 public:
  // Returns up to len bytes at offset, or 0 when the image ends there.
  using Reader = std::function<size_t(uint32_t offset, uint8_t *dst, size_t len)>;

  enum class State : uint8_t {
    IDLE,
    WAIT_READY,  // 0x8030 sent, waiting for the MCU's status + MTU
    TRANSFER,    // answering 0x8031 chunk pulls
    BURNING,     // download verified (0x8032), waiting for the flash result
    DONE,
    FAILED,
  };

  void begin(LevoitPlatform *platform) { platform_ = platform; }

  // Starts a transfer. major/minor/patch are the three bytes the MCU sees in
  // the 0x8030 frame (the stock firmware derives them from the cloud version
  // string, "V%d.%d.%02d"). Returns false if a transfer is already running.
  bool start(Reader reader, uint32_t size, uint8_t major, uint8_t minor, uint8_t patch);

  // Feeds one decoded frame. Returns true when the frame belongs to the DFU,
  // in which case the caller must not ACK or decode it as a status report.
  bool handle_frame(uint8_t msg_type, uint8_t seq, uint8_t op_lo, uint8_t op_hi,
                    const uint8_t *payload, size_t payload_len);

  // Inactivity timeout: the MCU gives up on a stalled transfer, so do we.
  void tick(uint32_t now);

  void abort();
  State state() const { return state_; }
  uint8_t progress() const;
  uint16_t mtu() const { return mtu_; }
  uint32_t size() const { return size_; }

 private:
  static constexpr uint8_t OP_HI = 0x80;
  static constexpr uint8_t OP_START = 0x30;
  static constexpr uint8_t OP_CHUNK = 0x31;
  static constexpr uint8_t OP_VERIFIED = 0x32;
  static constexpr uint8_t OP_BURNED = 0x33;
  static constexpr uint8_t OP_REBOOT_LO = 0x01;
  static constexpr uint8_t OP_REBOOT_HI = 0xD1;
  static constexpr uint16_t MTU_DEFAULT = 512;
  static constexpr uint16_t MTU_MAX = 1024;
  static constexpr uint32_t TIMEOUT_MS = 10000;

  void send_(const std::vector<uint8_t> &msg_type, const std::vector<uint8_t> &payload,
             uint8_t control, uint8_t counter);
  void send_start_();
  void send_reboot_();
  void send_ack_(uint8_t op_lo, uint8_t seq, const std::vector<uint8_t> &payload = {});
  void fail_(const char *why);

  LevoitPlatform *platform_{nullptr};
  Reader reader_;
  uint32_t size_{0};
  uint32_t offset_{0};
  uint16_t mtu_{MTU_DEFAULT};
  uint8_t version_[3]{0, 0, 0};
  uint8_t counter_{16};
  State state_{State::IDLE};
  uint32_t last_activity_ms_{0};
};

}  // namespace levoit
