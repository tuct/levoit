#include "mcu_dfu.h"

#include <cstdio>
#include <utility>

#include "levoit_log.h"
#include "levoit_message.h"

namespace levoit {

static const char *const TAG_DFU = "levoit.mcu_dfu";

static uint32_t read_le32_(const uint8_t *p) {
  return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
         ((uint32_t) p[3] << 24);
}

void McuDfu::send_(const std::vector<uint8_t> &msg_type,
                   const std::vector<uint8_t> &payload, uint8_t control,
                   uint8_t counter) {
  if (this->platform_ == nullptr)
    return;
  std::vector<uint8_t> frame = build_levoit_message(msg_type, payload, counter, control);
  char hex[3 * 64 + 1];
  size_t pos = 0;
  for (size_t i = 0; i < frame.size() && i < 24 && pos + 4 < sizeof(hex); i++) {
    pos += snprintf(hex + pos, sizeof(hex) - pos, "%02X ", frame[i]);
  }
  ESP_LOGD(TAG_DFU, ">>> TX %s (%u bytes)", hex, (unsigned) frame.size());
  this->platform_->write(frame.data(), frame.size());
}

void McuDfu::send_start_() {
  // 0x8030 payload: channel, version length, major, minor, patch, size LE32,
  // then the two 0xFF flag bytes the stock firmware sends.
  std::vector<uint8_t> payload = {0x00, 0x04, this->version_[0], this->version_[1],
                                  this->version_[2]};
  const uint32_t size = this->size_;
  payload.push_back((uint8_t) (size & 0xFF));
  payload.push_back((uint8_t) ((size >> 8) & 0xFF));
  payload.push_back((uint8_t) ((size >> 16) & 0xFF));
  payload.push_back((uint8_t) ((size >> 24) & 0xFF));
  payload.push_back(0xFF);
  payload.push_back(0xFF);
  this->send_({0x01, OP_START, OP_HI}, payload, 0x22, this->counter_++);
  if (this->counter_ == 0)
    this->counter_ = 16;
}

void McuDfu::send_ack_(uint8_t op_lo, uint8_t seq, const std::vector<uint8_t> &payload) {
  // ACKs echo the request's sequence number, as ackMessage() does.
  this->send_({0x01, op_lo, OP_HI}, payload, 0x12, seq);
}

void McuDfu::send_reboot_() {
  this->send_({0x01, OP_REBOOT_LO, OP_REBOOT_HI}, {0x01}, 0x22, this->counter_++);
  if (this->counter_ == 0)
    this->counter_ = 16;
}

bool McuDfu::start(Reader reader, uint32_t size, uint8_t major, uint8_t minor,
                   uint8_t patch) {
  if (this->platform_ == nullptr || !reader || size == 0)
    return false;
  if (this->state_ == State::WAIT_READY || this->state_ == State::TRANSFER ||
      this->state_ == State::BURNING)
    return false;
  this->reader_ = std::move(reader);
  this->size_ = size;
  this->offset_ = 0;
  this->mtu_ = MTU_DEFAULT;
  this->version_[0] = major;
  this->version_[1] = minor;
  this->version_[2] = patch;
  this->state_ = State::WAIT_READY;
  this->last_activity_ms_ = this->platform_->millis();
  ESP_LOGI(TAG_DFU, "Start MCU DFU: %u bytes, version %u.%u.%02u", (unsigned) size,
           (unsigned) major, (unsigned) minor, (unsigned) patch);
  this->send_start_();
  return true;
}

bool McuDfu::handle_frame(uint8_t msg_type, uint8_t seq, uint8_t op_lo, uint8_t op_hi,
                          const uint8_t *payload, size_t payload_len) {
  if (this->state_ == State::IDLE || this->state_ == State::DONE ||
      this->state_ == State::FAILED)
    return false;
  if (op_hi != OP_HI)
    return false;

  switch (op_lo) {
    case OP_START: {
      if (msg_type != 0x12 || this->state_ != State::WAIT_READY)
        return false;
      const uint8_t status = payload_len > 0 ? payload[0] : 0xFF;
      if (status != 0) {
        this->fail_("MCU refused the image (0x8030 status)");
        return true;
      }
      if (payload_len >= 3) {
        const uint16_t mtu = (uint16_t) payload[1] | ((uint16_t) payload[2] << 8);
        if (mtu >= 64 && mtu <= MTU_MAX)
          this->mtu_ = mtu;
      }
      this->state_ = State::TRANSFER;
      this->last_activity_ms_ = this->platform_->millis();
      ESP_LOGI(TAG_DFU, "MCU ready, MTU %u", (unsigned) this->mtu_);
      return true;
    }

    case OP_CHUNK: {
      if (msg_type != 0x22 || this->state_ != State::TRANSFER)
        return false;
      if (payload_len < 4) {
        this->fail_("Malformed 0x8031 request");
        return true;
      }
      const uint32_t offset = read_le32_(payload);
      if (offset >= this->size_) {
        this->fail_("MCU asked past the end of the image");
        return true;
      }
      size_t want = this->mtu_;
      if (this->size_ - offset < want)
        want = this->size_ - offset;
      std::vector<uint8_t> data(want);
      const size_t got = this->reader_(offset, data.data(), want);
      if (got != want) {
        this->fail_("Image read failed (short read)");
        return true;
      }
      this->offset_ = offset + (uint32_t) got;
      const bool last = this->offset_ >= this->size_;
      std::vector<uint8_t> reply;
      reply.reserve(7 + got);
      reply.push_back(last ? 0x01 : 0x00);
      reply.push_back((uint8_t) (offset & 0xFF));
      reply.push_back((uint8_t) ((offset >> 8) & 0xFF));
      reply.push_back((uint8_t) ((offset >> 16) & 0xFF));
      reply.push_back((uint8_t) ((offset >> 24) & 0xFF));
      reply.push_back((uint8_t) (got & 0xFF));
      reply.push_back((uint8_t) ((got >> 8) & 0xFF));
      reply.insert(reply.end(), data.begin(), data.end());
      this->send_ack_(OP_CHUNK, seq, reply);
      this->last_activity_ms_ = this->platform_->millis();
      ESP_LOGV(TAG_DFU, "0x8031 offset %u len %u%s", (unsigned) offset,
               (unsigned) got, last ? " (last)" : "");
      return true;
    }

    case OP_VERIFIED: {
      if (msg_type != 0x22)
        return false;
      const uint8_t status = payload_len > 0 ? payload[0] : 0xFF;
      this->send_ack_(OP_VERIFIED, seq);
      this->last_activity_ms_ = this->platform_->millis();
      if (status != 0) {
        this->fail_("MCU download verification failed");
        return true;
      }
      this->state_ = State::BURNING;
      ESP_LOGI(TAG_DFU, "MCU verified the download; burning");
      return true;
    }

    case OP_BURNED: {
      if (msg_type != 0x22)
        return false;
      const uint8_t status = payload_len > 0 ? payload[0] : 0xFF;
      this->send_ack_(OP_BURNED, seq);
      if (status != 0) {
        this->fail_("MCU flash write failed");
        return true;
      }
      this->state_ = State::DONE;
      ESP_LOGI(TAG_DFU, "MCU flash OK, rebooting it");
      this->send_reboot_();
      return true;
    }

    default:
      return false;
  }
}

void McuDfu::tick(uint32_t now) {
  if (this->state_ == State::WAIT_READY || this->state_ == State::TRANSFER ||
      this->state_ == State::BURNING) {
    if (now - this->last_activity_ms_ > TIMEOUT_MS)
      this->fail_("DFU timed out (no progress)");
  }
}

void McuDfu::abort() {
  if (this->state_ == State::DONE)
    return;
  this->state_ = State::IDLE;
  this->offset_ = 0;
}

void McuDfu::fail_(const char *why) {
  ESP_LOGE(TAG_DFU, "%s", why);
  this->state_ = State::FAILED;
}

uint8_t McuDfu::progress() const {
  if (this->state_ == State::DONE)
    return 100;
  if (this->size_ == 0)
    return 0;
  uint32_t pct = (uint32_t) ((uint64_t) this->offset_ * 100 / this->size_);
  return pct > 99 ? 99 : (uint8_t) pct;
}

}  // namespace levoit
