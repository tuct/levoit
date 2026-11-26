#include "lv600s_humidifier.h"
#include "esphome/core/log.h"
#include <cstdio>

namespace esphome::lv600s_humidifier {
void LV600SHumidifier::setup() {
  protocol_.transmit = [this](const Bytes &frame) { write_array(frame.data(), frame.size()); flush(); };
  protocol_.query(); protocol_.query_timer();
  poll_at_ = millis();
}
void LV600SHumidifier::loop() {
  const uint32_t now = millis();
  while (available()) { uint8_t b; if (!read_byte(&b)) break; protocol_.feed(b, now); }
  protocol_.tick(now);
  if (uint32_t(now - poll_at_) >= interval_) {
    poll_at_ = now; protocol_.query(); protocol_.query_timer();
  }
  if (uint32_t(now - publish_at_) >= 1000) { publish_at_ = now; publish_(); }
}
void LV600SHumidifier::dump_config() {
  ESP_LOGCONFIG("lv600s", "LV600S MCU UART, 9600 8N1; component " LV600S_VERSION);
}
std::string LV600SHumidifier::get_mode_name() const {
  if (!has_status()) return "Unknown";
  switch (uint8_t(get_mode())) {
    case 0: return "Auto (read only)";
    case 1: return "Manual";
    case 2: return "Humidity Sleep";
    case 3: return "Humidity";
    default: return "Unknown";
  }
}
void LV600SHumidifier::publish_() {
  const auto &s = protocol_.status; const auto &v = s.raw; const bool fresh = has_status();
  const float values[] = {float(v[9]), float(v[10]), get_target_humidity(), float(v[12]), float(v[13]),
                         float(v[6]), float(v[11]), float(v[7]), float(v[4]), float(v[14]),
                         float(v[15]), get_timer_remaining()};
  for (size_t i = 0; i < sensors_.size(); ++i) if (sensors_[i]) sensors_[i]->publish_state(fresh ? values[i] : NAN);
  const bool flags[] = {s.power(), s.display(), s.water_empty(), s.tank_removed(), s.humidifying(),
                        s.heating_enabled(), fresh, v[15] != 0};
  for (size_t i = 0; i < binaries_.size(); ++i) {
    if (!binaries_[i]) continue;
    if (fresh || i == 6) binaries_[i]->publish_state(flags[i]);
    else binaries_[i]->invalidate_state();
  }
  char version[24]; std::snprintf(version, sizeof(version), "%u.%u.%02u", v[2], v[1], v[0]);
  char raw[61]{};
  for (size_t i = 0; i < s.size; ++i) std::snprintf(raw + i * 3, 4, "%02X ", v[i]);
  static constexpr const char *FAULTS[] = {
      "No reported MCU fault", "E1: Dry-run protection (11601000)", "E2: PTC heater (11602000)",
      "E3: Overtemperature (11603000)", "E4: Water fault (11604000)", "MCU code 5 (11014000)",
      "E6: Communication fault (11607000)",
  };
  char unknown_fault[48];
  const char *fault;
  if (v[15] < sizeof(FAULTS) / sizeof(FAULTS[0])) {
    fault = FAULTS[v[15]];
  } else {
    std::snprintf(unknown_fault, sizeof(unknown_fault), "Unknown MCU fault (raw %u)", v[15]);
    fault = unknown_fault;
  }
  const std::string states[] = {fresh ? version : "Unknown", protocol_.result, fresh ? raw : "Unknown",
                                 fresh ? fault : "Unknown", LV600S_VERSION " esphome", "LV600S"};
  for (size_t i = 0; i < texts_.size(); ++i) if (texts_[i]) texts_[i]->publish_state(states[i]);
}
}  // namespace esphome::lv600s_humidifier
