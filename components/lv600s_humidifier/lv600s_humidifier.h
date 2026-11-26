#pragma once
#include "protocol.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#ifndef LV600S_VERSION
#define LV600S_VERSION "unknown"
#endif
namespace esphome::lv600s_humidifier {
class LV600SHumidifier : public Component, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_status_interval(uint32_t interval) { interval_ = interval; }
  void register_sensor(size_t kind, sensor::Sensor *s) { sensors_[kind] = s; }
  void register_binary_sensor(size_t kind, binary_sensor::BinarySensor *s) { binaries_[kind] = s; }
  void register_text_sensor(size_t kind, text_sensor::TextSensor *s) { texts_[kind] = s; }
  bool has_status() const { return protocol_.status.fresh(millis()); }
  bool has_timer() const { return protocol_.timer_known(millis()); }
  bool is_power_on() const { return protocol_.status.power(); }
  bool is_display_on() const { return protocol_.status.display(); }
  bool water_lacks() const { return protocol_.status.water_empty(); }
  bool is_tank_removed() const { return protocol_.status.tank_removed(); }
  bool is_humidifying() const { return protocol_.status.humidifying(); }
  bool is_warm_enabled() const { return protocol_.status.heating_enabled(); }
  float get_current_humidity() const { return protocol_.status.raw[9]; }
  float get_current_temperature() const { return protocol_.status.raw[10]; }
  float get_target_humidity() const {
    auto value = protocol_.status.raw[8];
    return has_status() && get_mode() != 1 && value >= 40 && value <= 80 ? value : NAN;
  }
  float get_mist_level() const { return protocol_.status.raw[12]; }
  float get_warm_level() const { return protocol_.status.raw[13]; }
  float get_mode() const { return protocol_.status.raw[11]; }
  float get_display_config() const { return protocol_.status.raw[6]; }
  float get_status_age() const { return protocol_.status.seen ? uint32_t(millis() - protocol_.status.received) / 1000.0f : NAN; }
  uint32_t get_rx_errors() const { return protocol_.rx_errors; }
  uint32_t get_command_failures() const { return protocol_.command_failures; }
  float get_timer_remaining() const { return protocol_.remaining(millis()); }
  std::string get_mode_name() const;
  void query_status() { protocol_.query(); }
  void set_power(bool on) { send_(Command::POWER, {uint8_t(on)}); }
  void set_display(bool on) { send_(Command::DISPLAY, {uint8_t(on ? 100 : 0)}); }
  void set_target_humidity(uint8_t target) { send_(Command::TARGET, {0, target}); }
  void set_mist_level(uint8_t level) { send_(Command::MIST, {level, 0, 0, 0}); }
  void set_warm_level(uint8_t level) { send_(Command::WARM, {level, 0, 0, 0, 0}); }
  void set_timer_seconds(uint32_t seconds) {
    send_(Command::TIMER, {uint8_t(seconds), uint8_t(seconds >> 8), uint8_t(seconds >> 16), uint8_t(seconds >> 24)});
  }
  void clear_timer() { set_timer_seconds(0); }
  void set_manual_mode() { send_(Command::MANUAL, {0, 1, uint8_t(std::max(1.0f, std::min(9.0f, get_mist_level())))}); }
  void set_target_humidity_mode() { send_(Command::HUMIDITY, {target_(), 0, 0, 0}); }
  void set_sleep_mode() { send_(Command::SLEEP, {target_(), 0, 0, 0, 0, 0}); }
  void set_panel_button_callback(void (*callback)(uint8_t)) { protocol_.panel_event = callback; }
  void set_wifi_led(uint8_t mode, uint16_t on_ms = 0, uint16_t off_ms = 0, uint8_t cycles = 0) {
    if (mode <= 2) protocol_.wifi({mode, uint8_t(on_ms), uint8_t(on_ms >> 8), uint8_t(off_ms), uint8_t(off_ms >> 8), cycles});
  }
 protected:
  Protocol protocol_;
  std::array<sensor::Sensor *, 12> sensors_{};
  std::array<binary_sensor::BinarySensor *, 8> binaries_{};
  std::array<text_sensor::TextSensor *, 6> texts_{};
  uint32_t interval_{5000}, poll_at_{0}, publish_at_{0};
  uint8_t target_() const { auto t = protocol_.status.raw[8]; return t >= 40 && t <= 80 ? t : 55; }
  void send_(Command command, Bytes payload) { protocol_.enqueue(command, std::move(payload), millis()); }
  void publish_();
};
}  // namespace esphome::lv600s_humidifier
