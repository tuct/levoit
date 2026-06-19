#pragma once
#include <vector>
#include <string>
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace philips {

// MUJI/Philips (Versuni) AC0650/AC0651 air purifier — MCU↔module UART protocol.
//   FE FF | cmd(LE16) | len(1) | data[len] | crc(BE16, CRC-16/CCITT-FALSE over data)
// Datapoints are addressed (group, dpid). See devices/philips-600-series/README.md.

enum class PhilipsModel : uint8_t {
  AC0650 = 0,  // base: no PM sensor, no Auto
  AC0651 = 1,  // adds PM sensor + Auto (not yet implemented)
};

enum class SensorType : uint8_t {
  FILTER_CLEAN = 0,     // pre-filter "clean" %  (group 0x05: DP 0x0D / 0x07)
  FILTER_LIFETIME = 1,  // HEPA replacement %    (group 0x05: DP 0x0E / 0x08)
  PM2_5 = 2,            // PM2.5 µg/m³ (AC0651 only — group 0x03: DP 0x21, PM1003 sensor)
  ALLERGEN_INDEX = 3,   // allergen / AQI index 1–12 (AC0651 only — group 0x03: DP 0x20)
};

enum class ButtonType : uint8_t {
  RESET_PREFILTER = 0,
  RESET_HEPA = 1,
};

enum class SwitchType : uint8_t {
  STANDBY_SENSOR = 0,   // keep the PM sensor monitoring on standby (group 0x03: DP 0x34)
};

enum class TextSensorType : uint8_t {
  MCU_VERSION = 0,   // MCU firmware version string from the group 0x01 device-info report
};

class PhilipsFan;
class PhilipsSensor;
class PhilipsSwitch;
class PhilipsTextSensor;

class Philips : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_model(PhilipsModel m) { model_ = m; }
  PhilipsModel get_model() const { return model_; }

  // registered from python codegen
  void set_fan(PhilipsFan *f) { fan_ = f; }
  void register_sensor(SensorType type, PhilipsSensor *s) { sensors_[(uint8_t) type] = s; }
  void register_switch(SwitchType type, PhilipsSwitch *sw) { switches_[(uint8_t) type] = sw; }
  void register_text_sensor(TextSensorType type, PhilipsTextSensor *t) { text_sensors_[(uint8_t) type] = t; }

  // control entry points (called by fan / switch / button entities)
  void set_power(bool on);
  void set_fan_mode(uint8_t mode_val);
  void set_switch(SwitchType type, bool state);
  void reset_filter(ButtonType which);
  void set_wifi_led(uint8_t state);  // 3 = connecting, 4 = connected

  // Fan-mode datapoint values (group 0x03, DP 0x0C)
  static constexpr uint8_t MODE_AUTO = 0x00;    // AC0651 only (preset)
  static constexpr uint8_t MODE_SLEEP = 0x11;
  static constexpr uint8_t MODE_MEDIUM = 0x01;
  static constexpr uint8_t MODE_TURBO = 0x12;

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
  static uint16_t crc16_(const uint8_t *data, size_t len);

  PhilipsModel model_{PhilipsModel::AC0650};
  PhilipsFan *fan_{nullptr};
  PhilipsSensor *sensors_[4]{nullptr, nullptr, nullptr, nullptr};
  PhilipsSwitch *switches_[1]{nullptr};
  PhilipsTextSensor *text_sensors_[1]{nullptr};

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
