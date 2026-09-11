#pragma once
#include <vector>
#include <string>
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace philips {

// MUJI/Philips (Versuni) air purifier — MCU↔module UART protocol.
//   FE FF | cmd(LE16) | len(1) | data[len] | crc(BE16, CRC-16/CCITT-FALSE over data)
// Datapoints are addressed (group, dpid). The 600 and 900 series speak the
// same protocol with the same datapoint numbering; the 900 adds datapoints and
// differs in two values (medium fan mode, HEPA total). See
// devices/philips-600-series/README.md and devices/philips-900-series/README.md.

enum class PhilipsModel : uint8_t {
  AC0650 = 0,  // 600 series, base: no PM sensor, no Auto
  AC0651 = 1,  // 600 series + PM sensor + Auto
  AC0950 = 2,  // 900 series, base
  AC0951 = 3,  // 900 series + PM sensor + Auto
};

enum class SensorType : uint8_t {
  FILTER_CLEAN = 0,      // pre-filter "clean" %  (group 0x05: DP 0x0D / 0x07)
  FILTER_LIFETIME = 1,   // HEPA replacement %    (group 0x05: DP 0x0E / 0x08)
  PM2_5 = 2,             // PM2.5 µg/m³ (sensor models — group 0x03: DP 0x21)
  ALLERGEN_INDEX = 3,    // allergen / AQI index 1–12 (sensor models — group 0x03: DP 0x20)
  TIMER_REMAINING = 4,   // minutes left on the sleep timer (900 — group 0x03: DP 0x11)
};

enum class ButtonType : uint8_t {
  RESET_PREFILTER = 0,
  RESET_HEPA = 1,
};

enum class SwitchType : uint8_t {
  STANDBY_SENSOR = 0,   // keep the PM sensor monitoring on standby (group 0x03: DP 0x34)
  CHILD_LOCK = 1,       // 900 only (group 0x03: DP 0x03)
  BEEP = 2,             // 900 only (group 0x03: DP 0x30 — 0 / 100, not 0 / 1)
};

enum class SelectType : uint8_t {
  DISPLAY_BRIGHTNESS = 0,  // 900 only (group 0x03: DP 0x04, mirrored to 0x05)
};

enum class NumberType : uint8_t {
  TIMER = 0,  // 900 only: sleep timer in hours (group 0x03: DP 0x10 index)
};

enum class TextSensorType : uint8_t {
  MCU_VERSION = 0,   // MCU firmware version string from the group 0x01 device-info report
};

class PhilipsFan;
class PhilipsSensor;
class PhilipsSwitch;
class PhilipsTextSensor;
class PhilipsSelect;
class PhilipsNumber;

class Philips : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_model(PhilipsModel m) { model_ = m; }
  PhilipsModel get_model() const { return model_; }

  // 900 series (AC0950/AC0951). Same protocol, but medium fan mode and the
  // HEPA total differ, and it exposes datapoints the 600 series does not.
  bool is_900() const {
    return this->model_ == PhilipsModel::AC0950 || this->model_ == PhilipsModel::AC0951;
  }
  // The "1" variants carry the PM sensor, which is also what gates the Auto preset.
  bool has_pm_sensor() const {
    return this->model_ == PhilipsModel::AC0651 || this->model_ == PhilipsModel::AC0951;
  }
  uint8_t mode_medium() const { return this->is_900() ? MODE_MEDIUM_900 : MODE_MEDIUM_600; }
  uint32_t hepa_total() const { return this->is_900() ? 9600 : 4800; }

  // registered from python codegen
  void set_fan(PhilipsFan *f) { fan_ = f; }
  void register_sensor(SensorType type, PhilipsSensor *s) { sensors_[(uint8_t) type] = s; }
  void register_switch(SwitchType type, PhilipsSwitch *sw) { switches_[(uint8_t) type] = sw; }
  void register_text_sensor(TextSensorType type, PhilipsTextSensor *t) { text_sensors_[(uint8_t) type] = t; }
  void register_select(SelectType type, PhilipsSelect *sel) { selects_[(uint8_t) type] = sel; }
  void register_number(NumberType type, PhilipsNumber *n) { numbers_[(uint8_t) type] = n; }

  // control entry points (called by fan / switch / button entities)
  void set_power(bool on);
  void set_fan_mode(uint8_t mode_val);
  void set_switch(SwitchType type, bool state);
  void reset_filter(ButtonType which);
  void set_wifi_led(uint8_t state);  // 3 = connecting, 4 = connected
  void set_display_brightness(uint8_t raw);  // 900: BRIGHTNESS_* value
  void set_timer_hours(uint8_t hours);       // 900: 0 = off, else 1–12 h

  // Fan-mode datapoint values (group 0x03, DP 0x0C). Medium is the one value
  // that differs between the series — use mode_medium(), not the raw constants.
  static constexpr uint8_t MODE_AUTO = 0x00;        // sensor models only (preset)
  static constexpr uint8_t MODE_SLEEP = 0x11;
  static constexpr uint8_t MODE_MEDIUM_600 = 0x01;
  static constexpr uint8_t MODE_MEDIUM_900 = 0x13;
  static constexpr uint8_t MODE_TURBO = 0x12;
  // Note: the reported speed (DP 0x0D) is 1–4, 0 when powered off, and 0x12 in
  // turbo — so it is not a plain level. Not exposed as an entity today.

  // Display brightness (900, group 0x03 DP 0x04). Three opaque constants, not a
  // 0–100 scale; the MCU mirrors the write to DP 0x05 on its own.
  static constexpr uint8_t BRIGHTNESS_OFF = 0x00;
  static constexpr uint8_t BRIGHTNESS_LOW = 0x73;
  static constexpr uint8_t BRIGHTNESS_BRIGHT = 0x7B;
  // Beep (900, group 0x03 DP 0x30) is stored as a percentage, driven as on/off.
  static constexpr uint8_t BEEP_OFF = 0;
  static constexpr uint8_t BEEP_ON = 100;

 protected:
  std::vector<uint8_t> build_frame_(uint16_t cmd, const std::vector<uint8_t> &data);
  void send_frame_(uint16_t cmd, const std::vector<uint8_t> &data);  // builds + enqueues
  void query_(uint8_t group);
  void set_group_(uint8_t group, const std::vector<uint8_t> &tlv);
  void handle_frame_(const std::vector<uint8_t> &frame);
  void handle_status_(const uint8_t *data, size_t len);
  void on_link_up_();  // first valid frame from MCU = handshake done
  void publish_sensor_(SensorType type, float value);
  void publish_switch_(SwitchType type, bool state);
  void publish_text_sensor_(TextSensorType type, const std::string &value);
  void publish_select_(SelectType type, const std::string &value);
  void publish_number_(NumberType type, float value);
  static uint16_t crc16_(const uint8_t *data, size_t len);

  PhilipsModel model_{PhilipsModel::AC0650};
  PhilipsFan *fan_{nullptr};
  PhilipsSensor *sensors_[5]{nullptr, nullptr, nullptr, nullptr, nullptr};
  PhilipsSwitch *switches_[3]{nullptr, nullptr, nullptr};
  PhilipsTextSensor *text_sensors_[1]{nullptr};
  PhilipsSelect *selects_[1]{nullptr};
  PhilipsNumber *numbers_[1]{nullptr};

  // RX frame parser
  std::vector<uint8_t> rx_;
  size_t expected_{0};

  // handshake / link state
  bool linked_{false};    // true once the MCU has answered (HS2 or any frame)
  uint32_t last_hs_{0};   // last HS1 send (for retry until linked)
  uint32_t hs_start_{0};  // when handshaking began (grace-period fallback)

  // lock-step TX: the MCU protocol is strict request→response. We queue frames
  // and send one at a time, waiting for the reply (like the stock module) — else
  // async SETs collide with the MCU's status traffic and get dropped.
  std::vector<std::vector<uint8_t>> tx_queue_;
  bool waiting_{false};
  uint32_t last_tx_{0};
  uint32_t last_rx_{0};  // last reply received — enforce a small gap before next TX

  // poll timing
  uint32_t last_poll_{0};
  uint32_t last_filter_poll_{0};
};

}  // namespace philips
}  // namespace esphome
