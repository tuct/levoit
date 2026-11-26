#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

namespace esphome::lv600s_humidifier {

// LV600S / LUH-A602S-WEUR, MCU 1.0.56. See the device's PROTOCOL.md.
// Pure C++ transport/state machine: also compiled by the native regression test.
using Bytes = std::vector<uint8_t>;
enum class Command : uint16_t {
  STATUS = 0x4110, REPORT = 0x4111, POWER = 0xA000, DISPLAY = 0xA105,
  MIST = 0x4113, WARM = 0x4112, TARGET = 0xA2E8, MANUAL = 0xA260,
  HUMIDITY = 0x4114, SLEEP = 0x4082, TIMER = 0xA264,
  TIMER_QUERY = 0xA265, TIMER_REPORT = 0xA266, WIFI = 0xA129,
};
inline uint32_t le32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline uint8_t checksum(const Bytes &data) {
  uint8_t sum = 0;
  for (size_t i = 0; i < data.size(); ++i) if (i != 5) sum += data[i];
  return uint8_t(~sum);
}
inline Bytes packet(Command command, const Bytes &payload, uint8_t sequence, uint8_t control = 0x22) {
  const uint16_t length = uint16_t(payload.size() + 4);
  const uint16_t code = static_cast<uint16_t>(command);
  Bytes out{0xA5, control, sequence, uint8_t(length), uint8_t(length >> 8), 0,
            1, uint8_t(code), uint8_t(code >> 8), 0};
  out.insert(out.end(), payload.begin(), payload.end());
  out[5] = checksum(out);
  return out;
}

struct Status {
  static constexpr uint32_t FRESH_MS = 15000;
  std::array<uint8_t, 20> raw{};
  size_t size{0};
  uint32_t received{0};
  bool seen{false};
  bool fresh(uint32_t now) const { return seen && uint32_t(now - received) < FRESH_MS; }
  bool power() const { return raw[3] != 0; }
  bool tank_removed() const { return raw[4] != 0; }
  bool water_empty() const { return raw[5] != 0; }
  bool safe() const { return !tank_removed() && !water_empty() && raw[15] == 0; }
  bool display() const { return power() && raw[6] != 0 && raw[11] != 2; }
  bool humidifying() const { return power() && safe() && raw[7] == 0; }
  bool heating_enabled() const { return power() && safe() && raw[13] != 0; }
};

class Protocol {
 public:
  Status status;
  uint32_t rx_errors{0}, command_failures{0}, revision{0};
  std::function<void(const Bytes &)> transmit;
  std::function<void(uint8_t)> panel_event;
  const char *result{"Waiting for MCU status"};

  void query() { write_(Command::STATUS, {}); }
  void query_timer() { write_(Command::TIMER_QUERY, {}); }
  void wifi(const Bytes &payload) { write_(Command::WIFI, payload); }
  bool timer_known(uint32_t now) const {
    if (!status.fresh(now) || !timer_seen_) return false;
    // The MCU may report only at start and expiry. While its status is fresh,
    // keep counting down and allow a short window for the final confirmation.
    return timer_seconds_ == 0 ||
           uint32_t(now - timer_received_) / 1000 < timer_seconds_ + 10;
  }
  float remaining(uint32_t now) const {
    if (!timer_known(now)) return NAN;
    if (timer_seconds_ == 0) return 0;
    const uint32_t elapsed = uint32_t(now - timer_received_) / 1000;
    // MCU confirmation is required for expiry, never invent an off/zero state.
    return float(timer_seconds_ > elapsed ? timer_seconds_ - elapsed : 1);
  }
  bool enqueue(Command command, Bytes payload, uint32_t now) {
    if (!allowed_(command, payload, now)) return fail_("Rejected: stale/unsafe status or mode");
    if (is_off_(command, payload)) { pending_.clear(); active_ = false; }
    if (pending_.size() >= 8) return fail_("Rejected: command queue full");
    pending_.push_back({command, std::move(payload), 0});
    tick(now);
    return true;
  }
  void tick(uint32_t now) {
    if (!status.fresh(now)) timer_seen_ = false;
    if (!rx_.empty() && uint32_t(now - rx_at_) > 250) { rx_.clear(); ++rx_errors; }
    discard_unsafe_(now);
    if (active_ && uint32_t(now - sent_) >= 2500) {
      auto &p = pending_.front();
      if (p.code != Command::TIMER && p.retries == 0 && allowed_(p.code, p.payload, now)) {
        ++p.retries;
      } else {
        pending_.pop_front();
        fail_("Timeout: target not confirmed by MCU");
      }
      active_ = false;
    }
    if (!active_ && !pending_.empty()) {
      auto &p = pending_.front();
      if (!allowed_(p.code, p.payload, now)) {
        pending_.pop_front(); fail_("Rejected: device state changed"); return;
      }
      write_(p.code, p.payload); sent_ = now; active_ = true; queried_ = false;
      result = "Sent: waiting for MCU target state";
    }
    if (active_ && !queried_ && uint32_t(now - sent_) >= 300) {
      query();
      if (pending_.front().code == Command::TIMER) query_timer();
      queried_ = true;
    }
  }
  void feed(uint8_t byte, uint32_t now) {
    if (!rx_.empty() && uint32_t(now - rx_at_) > 250) { rx_.clear(); ++rx_errors; }
    rx_at_ = now;
    if (rx_.empty() && byte != 0xA5) return;
    rx_.push_back(byte);
    while (rx_.size() >= 6) {
      if (rx_[0] != 0xA5) { rx_.erase(rx_.begin()); continue; }
      const size_t body = size_t(rx_[3]) | (size_t(rx_[4]) << 8);
      if (body < 4 || body > 512) { rx_.erase(rx_.begin()); ++rx_errors; continue; }
      if (rx_.size() < body + 6) return;
      Bytes frame(rx_.begin(), rx_.begin() + body + 6);
      if (frame[5] != checksum(frame)) { rx_.erase(rx_.begin()); ++rx_errors; continue; }
      rx_.erase(rx_.begin(), rx_.begin() + body + 6);
      receive_(frame, now);
    }
  }

 protected:
  struct Pending { Command code; Bytes payload; uint8_t retries; };
  Bytes rx_;
  std::deque<Pending> pending_;
  uint8_t sequence_{0};
  uint32_t rx_at_{0}, sent_{0}, timer_received_{0}, timer_seconds_{0};
  bool active_{false}, queried_{false}, timer_seen_{false};

  bool fail_(const char *message) { ++command_failures; result = message; return false; }
  static bool is_off_(Command c, const Bytes &p) {
    return c == Command::POWER && p.size() == 1 && p[0] == 0;
  }
  void discard_unsafe_(uint32_t now) {
    if (pending_.empty() || (status.fresh(now) && status.safe())) return;
    const bool keep_off = is_off_(pending_.front().code, pending_.front().payload);
    const size_t retained = keep_off ? 1 : 0;
    const size_t discarded = pending_.size() - retained;
    if (discarded == 0) return;
    pending_.erase(pending_.begin() + retained, pending_.end());
    if (!keep_off) active_ = false;
    command_failures += discarded;
    result = "Rejected: MCU or tank safety state changed";
  }
  void write_(Command command, const Bytes &payload) {
    if (transmit) transmit(packet(command, payload, sequence_++));
  }
  bool allowed_(Command c, const Bytes &p, uint32_t now) const {
    if (is_off_(c, p)) return true;
    if (!status.fresh(now) || !status.safe()) return false;
    switch (c) {
      case Command::POWER: return p == Bytes{1};
      case Command::DISPLAY: return p.size() == 1 && (p[0] == 0 || p[0] == 100) && status.power() && status.raw[11] != 2;
      case Command::TARGET: return p.size() == 2 && p[0] == 0 && p[1] >= 40 && p[1] <= 80 && status.power() && status.raw[11] == 3;
      case Command::MIST: return p.size() == 4 && p[0] >= 1 && p[0] <= 9 && status.raw[11] != 2;
      case Command::WARM: return p.size() == 5 && p[0] <= 3 && status.raw[11] != 2;
      case Command::MANUAL: return p.size() == 3 && p[0] <= 1 && p[1] <= 1 && p[2] >= 1 && p[2] <= 9;
      case Command::HUMIDITY: return p.size() == 4 && p[0] >= 40 && p[0] <= 80;
      case Command::SLEEP: return p.size() == 6 && p[0] >= 40 && p[0] <= 80;
      case Command::TIMER: return p.size() == 4 && le32(p.data()) <= 43200;
      default: return false;
    }
  }
  void receive_(const Bytes &f, uint32_t now) {
    // There is no evidence for reusing the purifiers' TLV decoder or ACK policy.
    if (f[6] > 1 || f[9] != 0) return;
    const bool send = f[1] == 0x02 || f[1] == 0x22;
    const bool reply = f[1] == 0x12;
    const uint16_t code = uint16_t(f[7]) | (uint16_t(f[8]) << 8);
    const size_t n = f.size() - 10;
    const uint8_t *p = f.data() + 10;
    if (send && n == 1 && ((code == 0xD102 && (p[0] == 2 || p[0] == 3)) ||
                          (code == 0xD103 && p[0] == 4))) {
      if (panel_event) panel_event(p[0]);
    }
    if ((code == uint16_t(Command::STATUS) && (send || reply) && (n == 16 || n == 20)) ||
        (code == uint16_t(Command::REPORT) && send && n == 20)) {
      // A resumed status stream does not validate the previous timer snapshot.
      if (!status.fresh(now)) {
        timer_seen_ = false;
        discard_unsafe_(now);
      }
      status.raw.fill(0);
      std::copy(p, p + n, status.raw.begin());
      status.size = n;
      status.seen = true; status.received = now; ++revision;
      discard_unsafe_(now);
      confirm_(now, false);
    } else if ((code == uint16_t(Command::TIMER_QUERY) && (send || reply) && (n == 2 || n == 4)) ||
               (code == uint16_t(Command::TIMER_REPORT) && send && n == 8)) {
      const uint32_t seconds = n == 2 ? uint32_t(p[0]) | (uint32_t(p[1]) << 8) : le32(p);
      if (seconds > 43200 || (n == 8 && (le32(p + 4) > 43200 || seconds > le32(p + 4)))) return;
      if (!status.fresh(now)) return;
      const bool confirms_write = timer_matches_(seconds, now);
      if (!timer_seen_ || seconds != timer_seconds_ || confirms_write) timer_received_ = now;
      timer_seconds_ = seconds; timer_seen_ = true;
      confirm_(now, true);
    }
    // ACKs alone do not prove that the appliance has applied a state change.
  }
  bool timer_matches_(uint32_t seconds, uint32_t now) const {
    if (!active_ || pending_.front().code != Command::TIMER) return false;
    const uint32_t target = le32(pending_.front().payload.data());
    if (target == 0) return seconds == 0;
    const uint32_t elapsed = uint32_t(now - sent_) / 1000;
    const uint32_t expected = target > elapsed ? target - elapsed : 0;
    return seconds > 0 && std::abs(int(seconds) - int(expected)) <= 2;
  }
  void confirm_(uint32_t now, bool timer) {
    if (!active_ || pending_.empty()) return;
    const auto &c = pending_.front(); const auto &p = c.payload;
    bool matches = false;
    if (timer) {
      matches = timer_matches_(timer_seconds_, now);
    } else {
      switch (c.code) {
        case Command::POWER: matches = status.power() == (p[0] != 0); break;
        case Command::DISPLAY: matches = status.raw[6] == p[0]; break;
        case Command::TARGET: matches = status.raw[8] == p[1]; break;
        case Command::MIST: matches = status.raw[12] == p[0]; break;
        case Command::WARM: matches = status.raw[13] == p[0]; break;
        case Command::HUMIDITY: matches = status.raw[11] == 3 && status.raw[8] == p[0]; break;
        case Command::SLEEP: matches = status.raw[11] == 2 && status.raw[8] == p[0]; break;
        case Command::MANUAL: matches = status.raw[11] == 1 && (!p[1] || status.raw[12] == p[2]) && (!p[0] || status.raw[13] == p[2]); break;
        default: break;
      }
    }
    if (matches) { active_ = false; pending_.pop_front(); result = "Target confirmed by MCU"; }
  }
};
}  // namespace esphome::lv600s_humidifier
