#include "philips.h"
#include "esphome/core/defines.h"
// Platform sub-directories are only copied into the build when that platform is
// actually configured, so each include (and any use of its concrete type) must
// be guarded by the matching USE_* macro — else e.g. an AC0650 config without a
// switch fails to find switch/philips_switch.h.
#ifdef USE_FAN
#include "fan/philips_fan.h"
#endif
#ifdef USE_SENSOR
#include "sensor/philips_sensor.h"
#endif
#ifdef USE_SWITCH
#include "switch/philips_switch.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "text_sensor/philips_text_sensor.h"
#endif
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace philips {

static const char *const TAG = "philips";

// A dotted numeric version string like "0.1.9" (digits and dots only, ≥1 dot).
static bool looks_like_version(const std::string &s) {
  if (s.empty()) return false;
  bool dot = false;
  for (char c : s) {
    if (c == '.') dot = true;
    else if (c < '0' || c > '9') return false;
  }
  return dot;
}

// The MCU drops a command sent too soon after its reply — the stock module
// leaves a gap. Without this, it only worked when verbose logging happened to
// add the delay. Polls are ≥1 s apart, so this is invisible to throughput.
static constexpr uint32_t PHILIPS_TX_GAP_MS = 30;

void Philips::setup() {
  // Queue the first handshake (module → MCU HS1); loop() pumps the queue and
  // re-sends HS1 until the MCU answers (recovers from boot races / MCU reset).
  this->send_frame_(0x0001, {0x03, 0x00});
  this->last_hs_ = millis();
  this->hs_start_ = this->last_hs_;
}

void Philips::dump_config() {
  ESP_LOGCONFIG(TAG, "Philips Air Purifier (model=%s)",
                this->model_ == PhilipsModel::AC0651 ? "AC0651" : "AC0650");
  this->check_uart_settings(115200);
}

void Philips::loop() {
  // --- receive / frame ---
  while (this->available()) {
    uint8_t b = this->read();
    if (this->rx_.empty()) {
      if (b == 0xFE) this->rx_.push_back(b);
      continue;
    }
    if (this->rx_.size() == 1) {  // expect 0xFF
      if (b == 0xFF) {
        this->rx_.push_back(b);
      } else {
        this->rx_.clear();
        if (b == 0xFE) this->rx_.push_back(b);
      }
      continue;
    }
    this->rx_.push_back(b);
    if (this->rx_.size() == 5)
      this->expected_ = 7 + this->rx_[4];  // FE FF cmd(2) len(1) data(len) crc(2)
    if (this->expected_ && this->rx_.size() >= this->expected_) {
      this->handle_frame_(this->rx_);
      this->rx_.clear();
      this->expected_ = 0;
    }
    if (this->rx_.size() > 512) {
      this->rx_.clear();
      this->expected_ = 0;
    }
  }

  uint32_t now = millis();

  // give up on a lost reply so a dropped frame can't deadlock the queue
  if (this->waiting_ && now - this->last_tx_ > 300)
    this->waiting_ = false;

  // --- enqueue periodic work (only when the queue is idle, so SETs from HA
  //     take priority and polls don't pile up) ---
  if (this->tx_queue_.empty()) {
    if (!this->linked_) {
      if (now - this->last_hs_ >= 1000) {
        this->last_hs_ = now;
        // HS1 retries; after a grace period also try a poll in case the MCU
        // answers queries without completing the handshake.
        if (now - this->hs_start_ < 5000)
          this->send_frame_(0x0001, {0x03, 0x00});
        else
          this->query_(0x03);
      }
    } else if (now - this->last_poll_ >= 1000) {
      this->last_poll_ = now;
      this->query_(0x03);  // operating state: power, fan mode, fan level
    } else if (now - this->last_filter_poll_ >= 10000) {
      this->last_filter_poll_ = now;
      this->query_(0x05);  // filters
    }
  }

  // --- pump: send one queued frame, then wait for its reply (lock-step).
  //     Hold off PHILIPS_TX_GAP_MS after the last reply so the MCU doesn't
  //     drop the next frame (the bug that only showed without verbose logs). ---
  if (!this->waiting_ && !this->tx_queue_.empty() && now - this->last_rx_ >= PHILIPS_TX_GAP_MS) {
    auto frame = this->tx_queue_.front();
    this->tx_queue_.erase(this->tx_queue_.begin());
    this->write_array(frame.data(), frame.size());
    this->flush();
    ESP_LOGV(TAG, "TX -> %s", format_hex_pretty(frame).c_str());
    this->waiting_ = true;
    this->last_tx_ = now;
  }
}

void Philips::on_link_up_() {
  this->linked_ = true;
  ESP_LOGI(TAG, "MCU link up");
  // Mirror the stock boot sequence: device-info query + module SW version,
  // then set the panel Wi-Fi LED solid (connected).
  this->query_(0x01);
  this->set_group_(0x01, {0x0F, 0x73, 0x06, '0', '.', '0', '.', '0', 0x00});
  this->set_wifi_led(4);
  // poll promptly
  this->last_poll_ = 0;
  this->last_filter_poll_ = 0;
}

uint16_t Philips::crc16_(const uint8_t *data, size_t len) {
  // CRC-16/CCITT-FALSE over the data payload (sent big-endian on the wire).
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t) data[i] << 8;
    for (int j = 0; j < 8; j++)
      crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
  }
  return crc;
}

std::vector<uint8_t> Philips::build_frame_(uint16_t cmd, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> f;
  f.reserve(7 + data.size());
  f.push_back(0xFE);
  f.push_back(0xFF);
  f.push_back(cmd & 0xFF);
  f.push_back((cmd >> 8) & 0xFF);
  f.push_back((uint8_t) data.size());
  for (uint8_t b : data) f.push_back(b);
  uint16_t crc = crc16_(data.data(), data.size());
  f.push_back((crc >> 8) & 0xFF);
  f.push_back(crc & 0xFF);
  return f;
}

void Philips::send_frame_(uint16_t cmd, const std::vector<uint8_t> &data) {
  // queue for lock-step transmission in loop() (do NOT write inline — see loop)
  this->tx_queue_.push_back(this->build_frame_(cmd, data));
}

void Philips::query_(uint8_t group) { this->send_frame_(0x0004, {group, 0x00}); }

void Philips::set_group_(uint8_t group, const std::vector<uint8_t> &tlv) {
  // SET data = <group> <TLVs…> 00
  std::vector<uint8_t> d;
  d.reserve(tlv.size() + 2);
  d.push_back(group);
  for (uint8_t b : tlv) d.push_back(b);
  d.push_back(0x00);
  this->send_frame_(0x0003, d);
}

void Philips::set_power(bool on) {
  ESP_LOGD(TAG, "set power %s", on ? "ON" : "OFF");
  this->set_group_(0x03, {0x02, 0x01, (uint8_t)(on ? 1 : 0)});
}

void Philips::set_fan_mode(uint8_t mode_val) {
  ESP_LOGD(TAG, "set fan mode 0x%02X", mode_val);
  this->set_group_(0x03, {0x0C, 0x01, mode_val});
}

void Philips::set_switch(SwitchType type, bool state) {
  if (type == SwitchType::STANDBY_SENSOR) {
    ESP_LOGD(TAG, "set standby-sensor %s", state ? "ON" : "OFF");
    this->set_group_(0x03, {0x34, 0x01, (uint8_t)(state ? 1 : 0)});
  }
}

void Philips::reset_filter(ButtonType which) {
  if (which == ButtonType::RESET_PREFILTER) {
    ESP_LOGD(TAG, "reset pre-filter");
    this->set_group_(0x05, {0x0D, 0x02, 0x02, 0xD0});  // → 720
  } else {
    ESP_LOGD(TAG, "reset HEPA filter");
    this->set_group_(0x05, {0x0E, 0x04, 0x00, 0x00, 0x12, 0xC0});  // → 4800
  }
}

void Philips::set_wifi_led(uint8_t state) {
  // group 0x02, DP 0x02 = wifi status (3 connecting / 4 connected) + DP 0x03 = 1
  this->set_group_(0x02, {0x02, 0x01, state, 0x03, 0x01, 0x01});
}

void Philips::handle_frame_(const std::vector<uint8_t> &f) {
  ESP_LOGV(TAG, "RX <- %s", format_hex_pretty(f).c_str());
  if (f.size() < 7) return;
  uint16_t cmd = f[2] | (f[3] << 8);
  uint8_t len = f[4];
  if (f.size() < (size_t)(5 + len + 2)) return;
  uint16_t calc = crc16_(&f[5], len);
  uint16_t wire = (f[5 + len] << 8) | f[5 + len + 1];
  if (calc != wire) {
    ESP_LOGW(TAG, "bad CRC (calc=%04X wire=%04X) %s", calc, wire, format_hex_pretty(f).c_str());
    return;
  }
  // A valid frame is the reply to whatever we last sent → bus is free again.
  this->waiting_ = false;
  this->last_rx_ = millis();  // start the inter-frame gap before the next TX
  // Any valid frame (HS2 0x0002, or a STATUS) means the MCU link is alive.
  if (!this->linked_) this->on_link_up_();
  if (cmd == 0x0007) this->handle_status_(&f[5], len);
}

void Philips::handle_status_(const uint8_t *d, size_t n) {
  if (n < 6) return;
  uint8_t group = d[1];  // header: 00 <group> 01 02 00 00

  bool have_power = false, have_mode = false, power = false;
  uint8_t mode = 0;
  uint32_t pf_total = 0, pf_rem = 0, hepa_total = 0, hepa_rem = 0;
  bool have_pf = false, have_hepa = false;
  std::string fw;  // MCU firmware string from the group 0x01 device-info report

  size_t i = 6;  // skip the 6-byte header
  while (i + 1 < n && d[i] != 0x00) {
    uint8_t dpid = d[i];
    // String TLV: <dpid> 0x73 <len> <ascii…>. The 0x73 type byte never collides
    // with an integer length (always 1/2/4), so this disambiguates cleanly.
    if (d[i + 1] == 0x73) {
      uint8_t sl = d[i + 2];
      if (i + 3 + sl > n) break;
      std::string s(reinterpret_cast<const char *>(&d[i + 3]), sl);
      while (!s.empty() && s.back() == '\0') s.pop_back();
      if (group == 0x01) {
        // Self-documenting: log each device-info field with its DP so the exact
        // firmware DP is visible. Type/brand are words, model has a '/', the MCU
        // firmware and "other version" are dotted numbers — keep the non-zero one.
        ESP_LOGD(TAG, "device-info DP 0x%02X = '%s'", dpid, s.c_str());
        if (looks_like_version(s) && (fw.empty() || fw == "0.0.0")) fw = s;
      }
      i += 3 + sl;
      continue;
    }
    uint8_t l = d[i + 1];
    if (i + 2 + l > n) break;
    uint32_t val = 0;
    for (uint8_t k = 0; k < l; k++) val = (val << 8) | d[i + 2 + k];  // big-endian
    if (group == 0x03) {
      if (dpid == 0x02) { have_power = true; power = val != 0; }
      else if (dpid == 0x0C) { have_mode = true; mode = (uint8_t) val; }
      else if (dpid == 0x21) this->publish_sensor_(SensorType::PM2_5, (float) val);          // AC0651 PM2.5 µg/m³
      else if (dpid == 0x20) this->publish_sensor_(SensorType::ALLERGEN_INDEX, (float) val);  // AC0651 allergen index 1–12
      else if (dpid == 0x34) this->publish_switch_(SwitchType::STANDBY_SENSOR, val != 0);     // AC0651 standby sensor monitoring
      // dpid 0x0D = current fan level (derived; not exposed yet)
    } else if (group == 0x05) {
      if (dpid == 0x07) pf_total = val;
      else if (dpid == 0x0D) { pf_rem = val; have_pf = true; }
      else if (dpid == 0x08) hepa_total = val;
      else if (dpid == 0x0E) { hepa_rem = val; have_hepa = true; }
    }
    i += 2 + l;
  }

#ifdef USE_FAN
  if (group == 0x03 && this->fan_ != nullptr)
    this->fan_->apply_state(have_power ? power : this->fan_->state,
                            have_mode ? mode : 0xFF);
#endif
  if (group == 0x05) {
    if (have_pf && pf_total > 0)
      this->publish_sensor_(SensorType::FILTER_CLEAN, 100.0f * pf_rem / pf_total);
    if (have_hepa && hepa_total > 0)
      this->publish_sensor_(SensorType::FILTER_LIFETIME, 100.0f * hepa_rem / hepa_total);
  }
  if (group == 0x01 && !fw.empty())
    this->publish_text_sensor_(TextSensorType::MCU_VERSION, fw);
}

void Philips::publish_sensor_(SensorType type, float value) {
#ifdef USE_SENSOR
  auto *s = this->sensors_[(uint8_t) type];
  if (s != nullptr) s->publish_state(value);
#endif
}

void Philips::publish_switch_(SwitchType type, bool state) {
#ifdef USE_SWITCH
  auto *s = this->switches_[(uint8_t) type];
  if (s != nullptr) s->publish_state(state);
#endif
}

void Philips::publish_text_sensor_(TextSensorType type, const std::string &value) {
#ifdef USE_TEXT_SENSOR
  auto *t = this->text_sensors_[(uint8_t) type];
  if (t != nullptr) t->publish_state(value);
#endif
}

}  // namespace philips
}  // namespace esphome
