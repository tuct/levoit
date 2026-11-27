#pragma once
// The ESPHome component: a thin shell around LevoitDevice (levoit_device.h).
// It reads the UART and feeds the core, implements LevoitHost by forwarding
// state changes to the entities, and LevoitPlatform with UARTDevice,
// Component::set_timeout, preferences, the time component and network::.
// The platform entities (switch/, number/, ...) call it, and it forwards to
// the core. See docs/shared-core.md, step 3.
#include "esphome/core/preferences.h"

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/application.h"
#include "types.h"
#include "esphome_ns.h"
#include "levoit_device.h"
#include "levoit_platform.h"
#include "levoit_state.h"
#ifdef USE_LEVOIT_TIME
#include "esphome/components/time/real_time_clock.h"
#endif

// Set by __init__.py from components/levoit/VERSION and the git commit,
// e.g. "1.7.0+3fa21c4" (see version_info.py).
#ifndef LEVOIT_VERSION
#define LEVOIT_VERSION "unknown"
#endif

namespace esphome {
namespace levoit_component {

// forward declaration
class LevoitSwitch;
class LevoitNumber;
class LevoitFan;
class LevoitSensor;
class LevoitSelect;
class LevoitTextSensor;
class LevoitBinarySensor;
class LevoitButton;
class LevoitSproutLight;

class Levoit : public Component, public uart::UARTDevice, public LevoitHost, public LevoitPlatform {
 public:
  Levoit() : device_(this, this) {}

  const char* get_version() const { return LEVOIT_VERSION " esphome"; }
  LevoitDevice &device() { return this->device_; }

  // called from python codegen
  void register_switch(SwitchType type, LevoitSwitch *sw);
  void register_number(NumberType type, LevoitNumber *nm);
  void register_sensor(SensorType type, LevoitSensor *se);
  void register_select(SelectType type, LevoitSelect *sl);
  void register_text_sensor(TextSensorType type, LevoitTextSensor *tsl);
  void register_binary_sensor(BinarySensorType type, class LevoitBinarySensor *bs);
  void register_button(ButtonType type, class LevoitButton *btn);
  void register_light(LevoitSproutLight *light) { sprout_light_ = light; }
  LevoitSproutLight *get_sprout_light() const { return sprout_light_; }
  void set_fan(LevoitFan *fan) { this->fan_ = fan; }
  LevoitFan *get_fan() const { return this->fan_; }
#ifdef USE_LEVOIT_TIME
  void set_time(time::RealTimeClock *clock) { this->time_ = clock; }
#endif
  void set_device_model(std::string model) { this->device_.set_device_model(std::move(model)); }

  void setup() override;
  void loop() override;
  void dump_config() override;
  void on_shutdown() override;

  // ---- forwarded to the core for the platform entities ----
  ModelType get_model() const { return this->device_.get_model(); }
  const std::string &get_model_name() const { return this->device_.get_model_name(); }
  bool has_mcu_filter_reset() const { return this->device_.has_mcu_filter_reset(); }
  void on_switch_command(SwitchType type, bool state) { this->device_.on_switch_command(type, state); }
  void on_number_command(NumberType type, float value) { this->device_.on_number_command(type, value); }
  void on_select_command(SelectType type, uint32_t value) { this->device_.on_select_command(type, value); }
  void on_fan_command(int power, int speed_level, int mode) { this->device_.on_fan_command(power, speed_level, mode); }
  void note_number_state(NumberType type, float value) { this->device_.note_number_state(type, value); }
  bool get_binary_sensor_state(BinarySensorType type) const { return this->device_.get_binary_sensor_state(type); }
  void publish_select(SelectType type, uint32_t value) { this->device_.publish_select(type, value); }
  void publish_filter_stats_now() { this->device_.publish_filter_stats_now(); }
  void sendCommand(CommandType commandType) { this->device_.sendCommand(commandType); }
  void sendSproutLightDirect(bool on, bool breathing, uint8_t bri_pct, uint16_t ct_k) {
    this->device_.sendSproutLightDirect(on, breathing, bri_pct, ct_k);
  }
  uint8_t  get_pending_led_bri() const { return this->device_.get_pending_led_bri(); }
  uint16_t get_pending_led_ct()  const { return this->device_.get_pending_led_ct(); }
  void set_used_cadr(uint32_t value) { this->device_.set_used_cadr(value); }
  void set_total_runtime(uint32_t value) { this->device_.set_total_runtime(value); }
  uint8_t get_dry_level_preference() const { return this->device_.get_dry_level_preference(); }
  void set_dry_level_preference(uint8_t level) { this->device_.set_dry_level_preference(level); }
  // ---- and for YAML lambdas on the hub id, as before the split ----
  void send_aqi_to_mcu(uint16_t aqi) { this->device_.send_aqi_to_mcu(aqi); }
  uint32_t get_used_cadr() const { return this->device_.get_used_cadr(); }
  uint32_t get_total_runtime() const { return this->device_.get_total_runtime(); }
  float calculate_filter_life_left_percent() const { return this->device_.calculate_filter_life_left_percent(); }
  uint32_t calculate_current_cadr_per_hour() const { return this->device_.calculate_current_cadr_per_hour(); }
  bool is_timer_active() const { return this->device_.is_timer_active(); }
  bool is_core_model() const { return this->device_.is_core_model(); }

  // ---- LevoitHost: a changed slot goes to its entity ----
  void switch_changed(SwitchType type, bool state) override;
  void number_changed(NumberType type, float value) override;
  void sensor_changed(SensorType type, float value) override;
  void select_changed(SelectType type, uint32_t value) override;
  void text_changed(TextSensorType type, const std::string &value) override;
  void binary_changed(BinarySensorType type, bool state) override;
  void fan_changed(int power, int speed, int mode) override;
  void sprout_light_changed(bool on, float brightness, float color_temp, bool breathing) override;

  // ---- LevoitPlatform: ESPHome's UART, clock, timeouts, preferences, time, network ----
  void write(const uint8_t *data, size_t len) override;
  uint32_t millis() override;
  void set_timeout(const char *name, uint32_t ms, std::function<void()> fn) override;
  void cancel_timeout(const char *name) override;
  bool load_u32(const char *key, uint32_t &out) override;
  void save_u32(const char *key, uint32_t value) override;
  bool has_clock() override;
  bool local_time(uint32_t &epoch_local) override;
  NetworkState network_state() override;

 protected:
  LevoitDevice device_;
  LevoitSwitch *switches_[SWITCH_SLOTS] {nullptr};
  LevoitNumber *numbers_[NUMBER_SLOTS] {nullptr};
  LevoitSensor *sensors_[SENSOR_SLOTS] {nullptr};
  LevoitSelect *selects_[SELECT_SLOTS] {nullptr};
  LevoitTextSensor *text_sensor_[TEXT_SLOTS] {nullptr};
  class LevoitBinarySensor *binary_sensors_[BINARY_SLOTS] {nullptr};
  class LevoitButton *buttons_[4] {nullptr};
  LevoitFan *fan_{nullptr};
  LevoitSproutLight *sprout_light_{nullptr};
#ifdef USE_LEVOIT_TIME
  time::RealTimeClock *time_{nullptr};
#endif
  // Saved counters: one preference object per key the core asks for
  ESPPreferenceObject pref_filter_seconds_;
  ESPPreferenceObject pref_used_cadr_;
  ESPPreferenceObject pref_total_runtime_;
  ESPPreferenceObject *pref_for_(const char *key);
  // helper to convert enum to index
  static constexpr uint8_t st_idx_(SwitchType t) { return (uint8_t)t; }
  static constexpr uint8_t nt_idx_(NumberType t) { return (uint8_t)t; }
  static constexpr uint8_t st_idx_(SensorType t) { return (uint8_t)t; }
  static constexpr uint8_t sl_idx_(SelectType t) { return (uint8_t)t; }
  static constexpr uint8_t tsl_idx_(TextSensorType t) { return (uint8_t)t; }
  static constexpr uint8_t bs_idx_(BinarySensorType t) { return (uint8_t)t; }
};

}  // namespace levoit_component
}  // namespace esphome
