#pragma once
#include "lv600s_startup.h"
#include "protocol.h"
#include "lv600s_controls.h"
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
class LV600SHumidifier : public Component, public uart::UARTDevice, public ::levoit::lv600s::Controls {
 public:
  LV600SHumidifier() : Controls([] { return millis(); }) {}
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_status_interval(uint32_t interval) { interval_ = interval; }
  void register_sensor(size_t kind, sensor::Sensor *s) { sensors_[kind] = s; }
  void register_binary_sensor(size_t kind, binary_sensor::BinarySensor *s) { binaries_[kind] = s; }
  void register_text_sensor(size_t kind, text_sensor::TextSensor *s) { texts_[kind] = s; }
  float get_status_age() const {
    return protocol_.status.seen ? uint32_t(millis() - protocol_.status.received) / 1000.0f : NAN;
  }
  uint32_t get_rx_errors() const { return protocol_.rx_errors; }
  uint32_t get_command_failures() const { return protocol_.command_failures; }
 protected:
  std::array<sensor::Sensor *, 12> sensors_{};
  std::array<binary_sensor::BinarySensor *, 8> binaries_{};
  std::array<text_sensor::TextSensor *, 6> texts_{};
  uint32_t interval_{5000}, poll_at_{0}, publish_at_{0};
  // What publish_() last sent, so it only publishes a change (1 s sampling).
  bool published_{false};
  std::array<float, 12> published_values_{};
  std::array<int8_t, 8> published_flags_{};  // 0/1, or -1 for an invalidated state
  std::array<std::string, 6> published_texts_{};
  static bool same_(float a, float b) { return (std::isnan(a) && std::isnan(b)) || a == b; }
  void publish_();
};
}  // namespace esphome::lv600s_humidifier
