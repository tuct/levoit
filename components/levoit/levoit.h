#pragma once
#include "esphome/core/preferences.h"

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/application.h"
#include "types.h"

namespace esphome {
namespace levoit {

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





class Levoit : public Component, public uart::UARTDevice {
 public:
  const char* get_version() const { return "1.4.0 esphome"; }

  // called from python codegen
  void register_switch(SwitchType type, LevoitSwitch *sw);
  void register_number(NumberType type, LevoitNumber *nm);
  void register_sensor(SensorType type, LevoitSensor *se);
  void register_select(SelectType type, LevoitSelect *sl);
  void register_text_sensor(TextSensorType type, LevoitTextSensor *tsl);
  
  // called by switch, number,... when HA toggles it
  void on_switch_command(SwitchType type, bool state);
  void on_number_command(NumberType type, uint32_t value);  
  void on_select_command(SelectType type, uint32_t value);

  // called by decoder when device reports status
  void publish_switch(SwitchType type, bool state);
  void publish_number(NumberType type, float value);
  void publish_sensor(SensorType type, uint32_t value);
  void publish_select(SelectType type, uint32_t value);
  void publish_text_sensor(TextSensorType type, const std::string &value);
  // binary sensor + button
  void register_binary_sensor(BinarySensorType type, class LevoitBinarySensor *bs);
  void register_button(ButtonType type, class LevoitButton *btn);
  void publish_binary_sensor(BinarySensorType type, bool state);
  // EverestAir: filter % pushed to the MCU panel (read by setFilterPercent builder)
  uint8_t get_pending_filter_pct() const { return pending_filter_pct_; }
  void set_pending_filter_pct(uint8_t pct) { pending_filter_pct_ = pct > 100 ? 100 : pct; }
  // Sprout LED ring light
  void register_light(LevoitSproutLight *light) { sprout_light_ = light; }
  LevoitSproutLight *get_sprout_light() const { return sprout_light_; }
  void publish_sprout_light(bool on, float brightness, float color_temp, bool breathing);
  // Send nightlight/breathing command with explicit values (bypasses number state read)
  void sendSproutLightDirect(bool on, bool breathing, uint8_t bri_pct, uint16_t ct_k);
  uint8_t  get_pending_led_bri() const { return pending_led_bri_; }
  uint16_t get_pending_led_ct()  const { return pending_led_ct_; }
  // Send calculated AQI value to MCU display (CMD=02 06 55)
  void send_aqi_to_mcu(uint16_t aqi);
  uint16_t get_pending_aqi() const { return pending_aqi_; }
  // helper to compute and publish filter stats immediately
  void publish_filter_stats_now();
  // EverestAir: push computed filter % to the MCU panel when it changes
  void push_filter_pct_if_changed(float filter_left);

      
        

 



  // called by fan when HA toggles it
  void on_fan_command(int power, int speed_level, int mode);

  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_device_model(std::string model);
  void process_message(uint8_t *msg, int len);
  void track_cadr_usage();  // Track CADR and runtime based on fan state
  uint32_t calculate_current_cadr_per_hour() const;  // Compute current CADR/h based on speed and model
  float calculate_filter_life_left_percent() const;  // Remaining filter life percentage (0-100 with decimals)
  void sendCommand(CommandType commandType);   // if CommandType exists in your project
  void ackMessage(uint8_t ptype0, uint8_t ptype1);
  void set_fan(LevoitFan *fan) { this->fan_ = fan; }
  LevoitFan *get_fan() const { return this->fan_; }
  LevoitNumber *get_number(NumberType type) const { return numbers_[nt_idx_(type)]; }
  LevoitSelect *get_select(SelectType type) const { return selects_[sl_idx_(type)]; }
  LevoitSwitch *get_switch(SwitchType type) const { return switches_[st_idx_(type)]; }
  class LevoitBinarySensor *get_binary_sensor(BinarySensorType type) const { return binary_sensors_[bs_idx_(type)]; }
  bool get_binary_sensor_state(BinarySensorType type) const { return binary_sensor_states_[bs_idx_(type)]; }
  void start_timer(){this->timer_active_ = true; this->timer_stop_pending_ = false;};
  void stop_timer(){this->timer_active_ = false;};
  bool is_timer_active() const { return this->timer_active_; };
  void set_timer_stop_pending(bool v) { this->timer_stop_pending_ = v; if (v) this->timer_stop_sent_at_ = millis(); }
  bool is_timer_stop_pending() const { return this->timer_stop_pending_; };
  
  // Getters for internally tracked values
  uint32_t get_used_cadr() const { return used_cadr_; }
  uint32_t get_total_runtime() const { return total_runtime_; }
  ModelType get_model() const { return model_; }
  
  // Setters for internally tracked values (auto-saves to preferences)
  void set_used_cadr(uint32_t value) {
    used_cadr_ = value;
    pref_used_cadr_.save(&used_cadr_);
  }
  void set_total_runtime(uint32_t value) {
    total_runtime_ = value;
    pref_total_runtime_.save(&total_runtime_);
  }

  // Bulk-prefs cache — mirrors the 12 status TLVs (0x18..0x23) the MCU
  // emits on every push for the sleep / quick-clean / white-noise /
  // daytime preference clusters. Used by the upcoming setBulkPrefs SET
  // command (Stage 3) to echo non-edited fields back to the MCU in a
  // single 12-TLV bulk write. Populated by `update_bulk_pref` from
  // `vital_status.cpp` cases 0x18..0x23. The cache is filled by the
  // first boot-time decode (see "ESPHome × MCU interaction — boot-time
  // FIFO retention decode" in docs/MCU_2.0.0_baseline.md); after that,
  // status pushes with identical payloads are dedup-skipped and the
  // cache stays valid for the entire session.
  //
  // seen_mask bit layout: bit i = status TLV (0x18 + i) has been seen
  // at least once since boot. valid() requires all 12 bits set.
  struct BulkPrefsCache {
    uint8_t  sleep_type{0};   // TLV 0x18, bit 0
    uint8_t  qc_enabled{0};   // TLV 0x19, bit 1
    uint16_t qc_min{0};       // TLV 0x1A, bit 2
    uint8_t  qc_fan{0};       // TLV 0x1B, bit 3
    uint8_t  wn_enabled{0};   // TLV 0x1C, bit 4
    uint16_t wn_min{0};       // TLV 0x1D, bit 5
    uint8_t  wn_fan{0};       // TLV 0x1E, bit 6
    uint8_t  sleep_fan{0};    // TLV 0x1F, bit 7
    uint16_t sleep_min{0};    // TLV 0x20, bit 8
    uint8_t  dt_enabled{0};   // TLV 0x21, bit 9
    uint8_t  dt_mode{0};      // TLV 0x22, bit 10
    uint8_t  dt_level{0};     // TLV 0x23, bit 11
    uint16_t seen_mask{0};
    bool valid() const { return (seen_mask & 0x0FFF) == 0x0FFF; }
  };
  const BulkPrefsCache &get_bulk_prefs() const { return bulk_prefs_; }
  void update_bulk_pref(uint8_t tlv_id, uint32_t value);

  protected:
  LevoitSwitch *switches_[16] {nullptr};   // max SwitchType index = 6 (LED_RING)
  // Bumped from [16] to [24] in Stage 5: max NumberType index is now 16
  // (DAYTIME_FAN_LEVEL). At [16], register_number for that slot would have
  // written out of bounds. [24] leaves headroom for future additions.
  LevoitNumber *numbers_[24] {nullptr};
  LevoitSensor *sensors_[16] {nullptr};    // max SensorType index = 8 (FAN_RPM)
  LevoitSelect *selects_[16] {nullptr};    // max SelectType index = 9 (SLEEP_PREFERENCE)
  LevoitTextSensor *text_sensor_[16] {nullptr};  // max TextSensorType index = 6 (SPROUT_EVENT)
  class LevoitBinarySensor *binary_sensors_[8] {nullptr};
  bool binary_sensor_states_[8] {false};
  class LevoitButton *buttons_[4] {nullptr};
  LevoitFan *fan_{nullptr};
  LevoitSproutLight *sprout_light_{nullptr};
  ModelType model_{ModelType::VITAL100S};
  // Last commanded LED values — set by sendSproutLightDirect, read by build_sprout_command
  uint8_t  pending_led_bri_{100};    // 0–100 percent
  uint16_t pending_led_ct_{3500};    // Kelvin
  // Last AQI value to push to MCU display — set by send_aqi_to_mcu, read by setSproutAqiScale
  uint16_t pending_aqi_{0};
  // Filter % to push to the EverestAir MCU panel — read by setFilterPercent,
  // set from calculate_filter_life_left_percent() via push_filter_pct_if_changed().
  uint8_t pending_filter_pct_{100};
  int last_sent_filter_pct_{-1};  // last % sent to MCU; -1 = none yet (dedup)
  bool display_on_{false};

  uint16_t cadr = 235;
  uint8_t buffer_[512];
  size_t buf_len_{0};

  bool timer_active_{true};
  bool timer_stop_pending_{false};  // poll requestTimerStatus until MCU confirms off
  uint32_t timer_stop_sent_at_{0};  // millis() when stop was sent, for polling delay
  bool wifi_led_solid_{false};
  bool wifi_led_blink_sent_{false};  // true after first blinking command sent during boot
  bool filter_led_on_{false};
  bool filter_blinking_{false};
  
  // Internal tracked values (persisted in preferences)
  uint32_t used_cadr_{0};
  uint32_t total_runtime_{0};
  ESPPreferenceObject pref_used_cadr_;
  ESPPreferenceObject pref_total_runtime_;
  BulkPrefsCache bulk_prefs_;
  // helper to convert enum to index
  static constexpr uint8_t st_idx_(SwitchType t) { return (uint8_t)t; }
  static constexpr uint8_t nt_idx_(NumberType t) { return (uint8_t)t; }
  static constexpr uint8_t st_idx_(SensorType t) { return (uint8_t)t; }
  static constexpr uint8_t sl_idx_(SelectType t) { return (uint8_t)t; }
  static constexpr uint8_t tsl_idx_(TextSensorType t) { return (uint8_t)t; }
  static constexpr uint8_t bs_idx_(BinarySensorType t) { return (uint8_t)t; }
};

}  // namespace levoit
}  // namespace esphome