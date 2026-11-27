#pragma once
// The protocol core: one Levoit device on a UART, without ESPHome. It frames
// and checks the bytes the MCU sends, hands each frame to the model's decoder
// (decoder.cpp), keeps the device state (levoit_state.h), builds and sends
// commands, runs the timers and the CADR and filter tracking, and drives the
// Wi-Fi and filter LEDs. It talks to the outside through two interfaces: a
// LevoitHost hears about state changes (the ESPHome entities, or Matter
// clusters), a LevoitPlatform supplies the UART, clock, timers and saved
// counters. See docs/shared-core.md, step 3.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "levoit_platform.h"
#include "levoit_state.h"
#include "types.h"

namespace levoit {

// Eleva 300X auto-mode configuration block (CMD 02 21 55), cached from the
// MCU's REPORT/ACK so partial writes can echo the keys they do not change.
struct ElevaAutoConfig {
  bool known{false};
  uint8_t profile{0};          // key 01 (meaning TBD, seen 1..3)
  uint8_t adaptive_start{0};   // key 02
  uint8_t dynamic_manual{0};   // key 03
  uint8_t dynamic_sleep{0};    // key 04
  uint8_t quiet_enabled{0};    // key 05
  uint8_t quiet_trigger{0};    // key 06: 0 = light sensor, 1 = time window
  uint32_t quiet_start{79200}; // key 07, seconds after midnight
  uint32_t quiet_end{21600};   // key 08, seconds after midnight
};

class LevoitDevice {
 public:
  LevoitDevice(LevoitHost *host, LevoitPlatform *platform) : host_(host), platform_(platform) {}

  // ---- what the platform glue calls ----
  // The configured `model:` string, e.g. "CORE300S"; before begin().
  void set_device_model(std::string model);
  // Boot: restore the counters, light the LEDs, ask the MCU what needs asking.
  void begin();
  // Bytes from the MCU, as many as arrived; frames are decoded right away.
  void receive(const uint8_t *data, size_t len, uint32_t now);
  // Once per main loop pass: LEDs, timers, CADR, filter life.
  void tick(uint32_t now);
  // The platform's clock synced (Eleva 300X asks for the local time).
  void on_time_synced() { this->send_eleva_local_time(); }

  const std::string &get_model_name() const { return this->model_name_; }
  ModelType get_model() const { return model_; }
  const LevoitState &state() const { return this->state_; }
  // A number entity restored a saved value at boot: keep the state in step
  // without sending anything
  void note_number_state(NumberType type, float value) { this->state_.set_number(type, value); }
  bool get_binary_sensor_state(BinarySensorType type) const {
    bool state = false;
    this->state_.get_binary(type, state);
    return state;
  }

  // ---- what the host calls when the user acts ----
  void on_switch_command(SwitchType type, bool state);
  void on_number_command(NumberType type, float value);
  void on_select_command(SelectType type, uint32_t value);
  void on_fan_command(int power, int speed_level, int mode);
  // Send nightlight/breathing command with explicit values (bypasses number state read)
  void sendSproutLightDirect(bool on, bool breathing, uint8_t bri_pct, uint16_t ct_k);
  // Send calculated AQI value to MCU display (CMD=02 06 55)
  void send_aqi_to_mcu(uint16_t aqi);
  // Superior 6000S: Dry level is chosen up-front via a select, then applied when
  // Dry mode is picked on the fan entity; changes while drying apply immediately.
  // 0 = Low, 1 = High.
  uint8_t get_dry_level_preference() const { return dry_level_preference_; }
  bool has_dry_level_preference() const { return dry_level_preference_known_; }
  void set_dry_level_preference(uint8_t level) {
    dry_level_preference_ = level;
    dry_level_preference_known_ = true;
  }

  // Last decoded Superior drying countdown, in seconds.
  uint16_t get_dry_remaining() const { return dry_remaining_; }
  void set_dry_remaining(uint16_t seconds) { dry_remaining_ = seconds; }

  // ---- what the decoders call when the MCU reports ----
  // Records the value in the device state and, when it changed, hands it to
  // the host
  void publish_switch(SwitchType type, bool state);
  void publish_number(NumberType type, float value);
  void publish_sensor(SensorType type, float value);
  void publish_select(SelectType type, uint32_t value);
  void publish_text_sensor(TextSensorType type, const std::string &value);
  void publish_binary_sensor(BinarySensorType type, bool state);
  void publish_sprout_light(bool on, float brightness, float color_temp, bool breathing);
  // power/speed/mode are the same signed values the fan entity takes: -1
  // means "not reported in this frame, leave the field alone". Do not narrow
  // these - the Vital and Superior decoders rely on that sentinel.
  void apply_fan_status(int power, int speed, int mode);
  // The selected manual speed where the status carries it apart from the
  // running speed (FanState::selected_speed); call before apply_fan_status.
  void set_selected_speed(int speed) { this->state_.fan.selected_speed = speed; }
  // helper to compute and publish filter stats immediately
  void publish_filter_stats_now();
  // EverestAir: push computed filter % to the MCU panel when it changes
  void push_filter_pct_if_changed(float filter_left);
  void start_timer(){this->timer_active_ = true; this->timer_stop_pending_ = false;};
  void stop_timer(){this->timer_active_ = false;};
  bool is_timer_active() const { return this->timer_active_; };
  void set_timer_stop_pending(bool v) { this->timer_stop_pending_ = v; if (v) this->timer_stop_sent_at_ = platform_->millis(); }
  bool is_timer_stop_pending() const { return this->timer_stop_pending_; };
  // Superior 6000S runs its timer on the ESP: the MCU only accepts a remaining
  // value pushed to it, so the countdown itself lives here.
  void start_esp_timer(uint32_t duration_secs);
  void stop_esp_timer();
  void send_timer_update(uint32_t remaining_secs);
  void ackMessage(uint8_t ptype0, uint8_t ptype1, uint8_t sequence);
  void ackFilterReset(uint8_t ptype0, uint8_t ptype1);   // Superior 6000S: filter reset pressed on the panel
  void mirror_eleva_sensor_report(const uint8_t *payload, size_t payload_len);   // Eleva 300X: echo CMD 02 23 55 back to the MCU
  // Eleva 300X auto-mode config (02 21 55) and local time (02 22 55)
  ElevaAutoConfig &eleva_auto_config() { return eleva_auto_config_; }
  void apply_eleva_auto_config(const uint8_t *payload, size_t payload_len);
  void send_eleva_local_time();
  uint32_t get_pending_local_time() const { return pending_local_time_; }

  // ---- what the decoders and builders read ----
  // Each returns false when the slot is not known yet, leaving `out` untouched.
  bool try_get_number_state(NumberType type, float &out) const;
  bool try_get_select_index(SelectType type, size_t &out) const;
  bool try_get_switch_state(SwitchType type, bool &out) const;
  // EverestAir: filter % pushed to the MCU panel (read by setFilterPercent builder)
  uint8_t get_pending_filter_pct() const { return pending_filter_pct_; }
  void set_pending_filter_pct(uint8_t pct) { pending_filter_pct_ = pct > 100 ? 100 : pct; }
  uint8_t  get_pending_led_bri() const { return pending_led_bri_; }
  uint16_t get_pending_led_ct()  const { return pending_led_ct_; }
  uint16_t get_pending_aqi() const { return pending_aqi_; }
  // Filter accounting (#162). The filter wears by speed-weighted seconds: a
  // second at full CADR counts 1, a second at a speed that moves half the
  // air counts 0.5. Accrued on every tick, saved once per hour of it and in
  // shutdown(); a reset saves at once.
  uint32_t get_filter_seconds() const { return filter_seconds_; }
  // The same wear as cubic metres of air, the unit the old counter and the
  // Used CADR number use: seconds × CADR / 3600.
  uint32_t get_used_cadr() const {
    return (uint32_t)(((uint64_t) filter_seconds_ * cadr + 1800) / 3600);
  }
  uint32_t get_total_runtime() const { return total_runtime_; }
  // Set the wear in cubic metres (0 = new filter); saves at once.
  void set_used_cadr(uint32_t value) {
    this->set_filter_seconds((uint32_t)(((uint64_t) value * 3600 + cadr / 2) / cadr));
  }
  void set_filter_seconds(uint32_t seconds) {
    filter_seconds_ = seconds;
    filter_carry_ = 0;
    this->save_counters_(true);
  }
  void set_total_runtime(uint32_t value) {
    total_runtime_ = value;
    this->save_counters_(true);
  }
  // Save the counters now: the platform calls this when the firmware stops
  // (ESPHome's on_shutdown), so a power cut loses at most an hour.
  void shutdown() { this->save_counters_(true); }

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

  // decoder.cpp: the fingerprint of the last payload per status type, so an
  // unchanged periodic push is not decoded twice. One cache per device.
  struct PayloadHashCacheEntry {
    uint8_t ptype0;
    uint8_t ptype1;
    uint32_t hash;  // fingerprint of payload
    bool valid;
  };
  static constexpr size_t PAYLOAD_HASH_CACHE_SIZE = 16;
  PayloadHashCacheEntry (&payload_hash_cache())[PAYLOAD_HASH_CACHE_SIZE] { return payload_hash_cache_; }
  // After a command the state holds the requested value. While the MCU
  // applies it, a fast pusher still sends the old payload, so an unchanged
  // push within COMMAND_GRACE_MS of the command stays skipped. The first
  // unchanged push after that is decoded once: a command the MCU did not carry
  // out is then reported back instead of staying in the state.
  static constexpr uint32_t COMMAND_GRACE_MS = 1500;
  void note_command() {
    command_pending_ = true;
    command_ms_ = platform_->millis();
  }
  bool decode_unchanged_push() {
    if (!command_pending_ || platform_->millis() - command_ms_ < COMMAND_GRACE_MS)
      return false;
    command_pending_ = false;
    return true;
  }

  // ---- model knowledge ----
  // Core 200S / 300S / 400S / 600S: the Core protocol (it has a filter LED command)
  bool is_core_model() const {
    return this->model_ == ModelType::CORE200S || this->model_ == ModelType::CORE300S ||
           this->model_ == ModelType::CORE400S || this->model_ == ModelType::CORE600S;
  }
  // True where the MCU accepts a filter-counter reset of its own, in addition
  // to the ESP-side CADR counters. The Core200S does (01 E4 A5, captured from
  // the stock firmware); it does not report filter life back, so the ESP
  // estimate stays authoritative for the sensor. The Eleva 300X takes
  // 02 05 55 + 03 00, the frame its stock app sends.
  bool has_mcu_filter_reset() const { return this->model_ == ModelType::CORE200S || this->model_ == ModelType::ELEVA300X; }
  // Per-model facts a consumer would otherwise keep its own table of. Each
  // says why it is here:
  //   speed_count     - the fan's speed range to advertise (Turbo on the
  //                     EverestAir is a mode, not a speed, so it is not counted)
  //   has_auto        - whether to offer an Auto mode
  //   has_air_quality - whether the model reports PM2.5, i.e. whether to show it
  //   display_name    - a name to label the device with
  uint8_t speed_count() const {
    switch (this->model_) {
      case ModelType::CORE200S:
      case ModelType::CORE300S:
      case ModelType::EVERESTAIR:
        return 3;
      case ModelType::SUPERIOR6000S:
        return 9;
      default:
        return 4;
    }
  }
  bool has_auto() const { return this->model_ != ModelType::CORE200S; }
  bool has_air_quality() const {
    return this->model_ != ModelType::CORE200S && this->model_ != ModelType::SUPERIOR6000S;
  }
  // get_model_name() returns the configured key ("CORE300S"); this is "Core 300S".
  const char *display_name() const {
    switch (this->model_) {
      case ModelType::CORE200S: return "Core 200S";
      case ModelType::CORE300S: return "Core 300S";
      case ModelType::CORE400S: return "Core 400S";
      case ModelType::CORE600S: return "Core 600S";
      case ModelType::VITAL100S: return "Vital 100S";
      case ModelType::VITAL200S: return "Vital 200S";
      case ModelType::SPROUT: return "Sprout";
      case ModelType::EVERESTAIR: return "EverestAir";
      case ModelType::SUPERIOR6000S: return "Superior 6000S";
      case ModelType::ELEVA300X: return "Eleva 300X";
      default: return "Levoit";
    }
  }
  // Set the filter life to a percentage, the inverse of
  // calculate_filter_life_left_percent(): for a host that offers a "set filter
  // %" control (restoring the value the vendor app showed, say). Seeds the
  // used-CADR counter from the model's CADR and the configured lifetime, so a
  // consumer needs neither. Reset to new stays set_used_cadr(0).
  void set_filter_percent(uint8_t percent) {
    if (percent > 100)
      percent = 100;
    float months;
    if (!this->state_.get_number(NumberType::FILTER_LIFETIME_MONTHS, months) || months <= 0.0f)
      return;
    const uint64_t capacity = (uint64_t) 30 * 24 * 3600 * (uint32_t) months;  // seconds at full CADR
    this->set_filter_seconds((uint32_t)((capacity * (100 - percent) + 99) / 100));
    this->publish_filter_stats_now();
  }
  void sendCommand(CommandType commandType);
  void process_message(uint8_t *msg, int len);
  void track_cadr_usage();  // Track CADR and runtime based on fan state
  uint32_t calculate_current_cadr_per_hour() const;  // Compute current CADR/h based on speed and model
  float calculate_filter_life_left_percent() const;  // Remaining filter life percentage (0-100 with decimals)

  // Preference keys; the ESPHome platform hashes them the way the component
  // always did, so existing counters survive the move into the core.
  static constexpr const char *PREF_FILTER_SECONDS = "levoit_filter_seconds";
  static constexpr const char *PREF_TOTAL_RUNTIME = "levoit_runtime";
  // The counter before #162, cubic metres; read once to seed the seconds.
  static constexpr const char *PREF_USED_CADR = "levoit_used_cadr";

 protected:
  // Per-model rated CADR (m³/h); begin() seeds the counter from it and
  // set_filter_percent inverts the filter-life calc with it.
  uint16_t rated_cadr() const {
    switch (this->model_) {
      case ModelType::VITAL200S: return 415;
      case ModelType::VITAL100S: return 221;
      case ModelType::CORE300S: return 214;
      case ModelType::CORE400S: return 442;
      case ModelType::CORE200S: return 167;
      case ModelType::CORE600S: return 641;
      case ModelType::SPROUT: return 145;      // spec smoke CADR 144.5 m³/h (rounded)
      case ModelType::EVERESTAIR: return 612;
      case ModelType::SUPERIOR6000S: return 500;  // humidifier: nominal, feeds the CADR counters
      case ModelType::ELEVA300X: return 273;   // spec 161 CFM smoke/dust/PM2.5
      default: return 235;
    }
  }
  // Frame-parse everything complete in buffer_, resyncing on bad bytes
  void parse_rx_();
  // Core models: the filter LED follows FILTER_LOW
  void sync_filter_led_(float filter_left);
  // Filter accounting: add the time since the last tick, weighted by the
  // current CADR; save when an hour of wear or of runtime completes
  void accrue_filter_(uint32_t now);
  void save_counters_(bool force = false);

  LevoitHost *host_;
  LevoitPlatform *platform_;
  ModelType model_{ModelType::VITAL100S};
  std::string model_name_;
  // What the MCU reported, or what was last requested of it (levoit_state.h)
  LevoitState state_;
  // Last commanded LED values — set by sendSproutLightDirect, read by build_sprout_command
  uint8_t  pending_led_bri_{100};    // 0–100 percent
  uint16_t pending_led_ct_{3500};    // Kelvin
  // Last AQI value to push to MCU display — set by send_aqi_to_mcu, read by setSproutAqiScale
  uint16_t pending_aqi_{0};
  // Filter % to push to the EverestAir MCU panel — read by setFilterPercent,
  // set from calculate_filter_life_left_percent() via push_filter_pct_if_changed().
  uint8_t pending_filter_pct_{100};
  int last_sent_filter_pct_{-1};  // last % sent to MCU; -1 = none yet (dedup)
  // Eleva 300X
  ElevaAutoConfig eleva_auto_config_{};
  uint32_t pending_local_time_{0};   // local epoch for setElevaLocalTime
  bool fan_sleep_speed_{false};  // true when MCU reports FanLevel 0 (Sleep or Auto-low speed)

  uint16_t cadr = 235;
  uint8_t buffer_[512];
  uint32_t last_rx_byte_ms_{0};
  size_t buf_len_{0};

  // tick() bookkeeping, in platform milliseconds
  uint32_t last_check_{0};
  uint32_t last_check_sec_{0};
  uint32_t last_check_min_{30000};
  uint32_t last_cadr_check_{0};
  uint32_t last_filter_check_{0};

  bool timer_active_{true};
  bool timer_stop_pending_{false};  // poll requestTimerStatus until MCU confirms off
  uint32_t timer_stop_sent_at_{0};  // millis() when stop was sent, for polling delay
  enum class WifiLedState : uint8_t { UNKNOWN, OFF, BLINKING, SOLID };
  WifiLedState wifi_led_state_{WifiLedState::UNKNOWN};
  bool mcu_seen_{false};  // set on first valid RX frame from MCU
  bool fan_status_seen_{false};  // set on the first decoded status frame
  bool filter_led_on_{false};

  // Superior 6000S ESP-managed timer state
  bool esp_timer_active_{false};
  uint32_t esp_timer_start_millis_{0};
  uint32_t esp_timer_duration_secs_{0};
  uint32_t esp_timer_last_update_{0};
  uint8_t esp_timer_zero_count_{0};
  uint8_t dry_level_preference_{0};
  bool dry_level_preference_known_{false};
  uint16_t dry_remaining_{0};

  // Internal tracked values (persisted through the platform)
  uint32_t filter_seconds_{0};  // speed-weighted seconds of wear
  uint32_t total_runtime_{0};   // minutes the fan was on
  // Wear below one second, in ms × m³/h; not persisted, a reboot loses under
  // a second. uint64 so a long gap between ticks cannot overflow it.
  uint64_t filter_carry_{0};
  uint32_t accrue_ms_{0};       // platform time of the last accrual
  bool accrue_started_{false};
  uint32_t saved_filter_hours_{0};
  uint32_t saved_runtime_hours_{0};
  BulkPrefsCache bulk_prefs_;
  PayloadHashCacheEntry payload_hash_cache_[PAYLOAD_HASH_CACHE_SIZE]{};
  bool command_pending_{false};
  uint32_t command_ms_{0};
};

}  // namespace levoit
