#pragma once

#include <string>
#include <utility>

#include "lv600s_protocol.h"

namespace levoit::lv600s {

// The ESPHome wrappers and the core fan use the same model commands and
// matching rules. Only the platform supplies time and UART.
class Controls {
 public:
  explicit Controls(std::function<uint32_t()> clock) : clock_(std::move(clock)) {}

  bool has_status() const { return protocol_.status.fresh(clock_()); }
  bool has_timer() const { return protocol_.timer_known(clock_()); }
  bool has_timer_report() const { return protocol_.has_timer_report(); }
  bool can_apply_startup_setting() const {
    return has_status() && is_power_on() && protocol_.status.safe() &&
           !deferred_fan_ && protocol_.commands_idle();
  }
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
  float get_timer_remaining() const { return protocol_.remaining(clock_()); }
  std::string get_mode_name() const {
    if (!has_status()) return "Unknown";
    switch (uint8_t(get_mode())) {
      case 0: return "Auto (read only)";
      case 1: return "Manual";
      case 2: return "Humidity Sleep";
      case 3: return "Humidity";
      default: return "Unknown";
    }
  }
  void query_status() { protocol_.query(); }
  void set_power(bool on) {
    if (!on) deferred_fan_.reset();
    send_(Command::POWER, {uint8_t(on)});
  }
  void set_display(bool on) { send_(Command::DISPLAY, {uint8_t(on ? 100 : 0)}); }
  bool can_set_target_humidity() const {
    return has_status() && is_power_on() && get_mode() == 3 && protocol_.status.safe();
  }
  bool set_target_humidity(uint8_t target) {
    return protocol_.enqueue(Command::TARGET, {0, target}, clock_());
  }
  float get_timer_duration() const { return protocol_.timer_duration(clock_()); }
  static std::string format_timer_duration(float seconds) {
    if (!std::isfinite(seconds)) return "Unknown";
    if (seconds == 0) return "Off";
    const auto minutes = static_cast<unsigned>(std::ceil(seconds / 60.0f));
    if (minutes < 60) return std::to_string(minutes) + " min";
    return std::to_string(minutes / 60) + "h " + std::to_string(minutes % 60) + " min";
  }
  bool set_mist_level(uint8_t level) { return send_(Command::MIST, {level, 0, 0, 0}); }
  bool set_warm_level(uint8_t level) { return send_(Command::WARM, {level, 0, 0, 0, 0}); }
  bool set_timer_seconds(uint32_t seconds, uint32_t *confirmation_id = nullptr) {
    return protocol_.enqueue(Command::TIMER,
                             {uint8_t(seconds), uint8_t(seconds >> 8), uint8_t(seconds >> 16), uint8_t(seconds >> 24)},
                             clock_(), confirmation_id);
  }
  bool timer_write_confirmed(uint32_t id) const { return protocol_.command_confirmed(id); }
  void clear_timer() { set_timer_seconds(0); }
  bool set_manual_mode() {
    return set_manual_mist_level(uint8_t(std::max(1.0f, std::min(9.0f, get_mist_level()))));
  }
  bool set_manual_mist_level(uint8_t level) { return send_(Command::MANUAL, {0, 1, level}); }
  bool set_target_humidity_mode() { return send_(Command::HUMIDITY, {target_(), 0, 0, 0}); }
  bool set_sleep_mode() { return send_(Command::SLEEP, {target_(), 0, 0, 0, 0, 0}); }
  void set_fan_selection_callback(void (*callback)()) { fan_selection_ = callback; }
  void set_panel_button_callback(void (*callback)(uint8_t)) { protocol_.panel_event = callback; }
  void set_wifi_led(uint8_t mode, uint16_t on_ms = 0, uint16_t off_ms = 0, uint8_t cycles = 0) {
    if (mode <= 2) protocol_.wifi({mode, uint8_t(on_ms), uint8_t(on_ms >> 8), uint8_t(off_ms), uint8_t(off_ms >> 8), cycles});
  }

  void fan_command(int power, int speed, int mode) {
    if (power == 0) {
      set_power(false);
      return;
    }
    const bool settings = has_fan_selection_(speed, mode);
    if ((!settings && power != 1) || !has_status() || !protocol_.status.safe()) return;
    if (deferred_fan_) {
      const auto wake = deferred_fan_->wake_id;
      if (protocol_.command_pending(wake) || protocol_.command_confirmed(wake)) {
        // Several requests during one wake-up share its MCU confirmation. Keep the
        // latest explicit selection, so an earlier preset cannot override it.
        if (settings) deferred_fan_ = DeferredFan{speed, mode, wake};
        return;
      }
      deferred_fan_.reset();
    }
    if (!is_power_on()) {
      uint32_t wake = 0;
      if (protocol_.enqueue(Command::POWER, {1}, clock_(), &wake))
        deferred_fan_ = DeferredFan{speed, mode, wake};
    } else if (settings) {
      apply_fan_settings_(speed, mode);
    }
  }
  void tick(uint32_t now) {
    protocol_.tick(now);
    if (!deferred_fan_) return;
    const auto request = *deferred_fan_;
    if (protocol_.command_pending(request.wake_id)) return;
    deferred_fan_.reset();
    if (has_status() && is_power_on() && protocol_.status.safe() &&
        protocol_.command_confirmed(request.wake_id) && has_fan_selection_(request.speed, request.mode))
      apply_fan_settings_(request.speed, request.mode);
  }
  bool receive_frame(const uint8_t *frame, size_t size, uint32_t now) {
    const auto previous = protocol_.revision;
    protocol_.receive_frame(frame, size, now);
    if (protocol_.revision == previous) return false;
    // In a burst, the next status can be safe again before tick() runs.
    // Cancel the selection on the unsafe frame, even after power confirmed.
    if (deferred_fan_ && (!protocol_.status.safe() ||
        (!is_power_on() && protocol_.command_confirmed(deferred_fan_->wake_id))))
      deferred_fan_.reset();
    return true;
  }
  Protocol &protocol() { return protocol_; }

 protected:
  Protocol protocol_;

 private:
  struct DeferredFan { int speed; int mode; uint32_t wake_id; };
  std::function<uint32_t()> clock_;
  std::optional<DeferredFan> deferred_fan_;
  void (*fan_selection_)(){nullptr};

  uint8_t target_() const {
    const auto value = protocol_.status.raw[8];
    return value >= 40 && value <= 80 ? value : 55;
  }
  bool send_(Command command, Bytes payload) { return protocol_.enqueue(command, std::move(payload), clock_()); }
  static bool has_fan_selection_(int speed, int mode) {
    return (speed >= 1 && speed <= 9) || mode == 0 || mode == 1 || mode == 3;
  }
  void apply_fan_settings_(int speed, int mode) {
    bool accepted;
    if (mode == 1) accepted = set_sleep_mode();
    else if (mode == 3) accepted = set_target_humidity_mode();
    else if (speed >= 1 && speed <= 9) accepted = set_manual_mist_level(static_cast<uint8_t>(speed));
    else accepted = set_manual_mode();
    if (accepted && fan_selection_) fan_selection_();
  }
};

}  // namespace levoit::lv600s
