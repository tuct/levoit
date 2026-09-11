#include "levoit.h"
#include "levoit_message.h"
#include "esphome/components/network/util.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/components/wifi/wifi_component.h"

#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <time.h>
#include <vector>
#include <functional>
#include <algorithm>
#include "freertos/task.h"
#include "decoder.h"
#include "tlv.h"
#include "vital_status.h"
#include "core_status.h"
#include "core_commands.h"
#include "vital_commands.h"
#include "sprout_commands.h"
#include "superior_commands.h"
#include "decoder_helpers.h"   // format_duration_minutes
#include "sprout_status.h"
#ifdef USE_LIGHT
#include "light/levoit_light.h"
#endif
#ifdef USE_SWITCH
#include "switch/levoit_switch.h"
#endif
#include "fan/levoit_fan.h"
#ifdef USE_NUMBER
#include "number/levoit_number.h"
#endif
#ifdef USE_SENSOR
#include "sensor/levoit_sensor.h"
#endif
#ifdef USE_SELECT
#include "select/levoit_select.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "text_sensor/levoit_text_sensor.h"
#endif

namespace esphome
{
    namespace levoit
    {

        static const char *const TAG = "levoit";

        static const char *fan_operating_mode_to_option_(uint32_t mode)
        {
            switch (mode)
            {
            case 0:
                return "Manual";
            case 1:
                return "Sleep";
            case 2:
                return "Auto";
            case 4:
                return "Turbo";
            case 5:
                return "Pet";
            default:
                return nullptr;
            }
        }

        static bool supports_auto_fan_mode_(ModelType model)
        {
            return model != ModelType::CORE200S;
        }

        // ===== Component =====

        void Levoit::register_switch(SwitchType type, LevoitSwitch *sw)
        {
            if (!sw)
                return;
            switches_[st_idx_(type)] = sw;
        }
        void Levoit::register_number(NumberType type, LevoitNumber *nm)
        {
            if (!nm)
                return;
            numbers_[nt_idx_(type)] = nm;
        }
        void Levoit::register_sensor(SensorType type, LevoitSensor *se)
        {
            if (!se)
                return;
            sensors_[st_idx_(type)] = se;
        }
        void Levoit::register_select(SelectType type, LevoitSelect *sl)
        {
            if (!sl)
                return;
            selects_[sl_idx_(type)] = sl;
        }
        void Levoit::register_text_sensor(TextSensorType type, LevoitTextSensor *tsl)
        {
            if (!tsl)
                return;
            text_sensor_[static_cast<uint8_t>(type)] = tsl;
        }
        void Levoit::register_binary_sensor(BinarySensorType type, LevoitBinarySensor *bs)
        {
            if (!bs)
                return;
            binary_sensors_[bs_idx_(type)] = bs;
        }
        void Levoit::register_button(ButtonType type, LevoitButton *btn)
        {
            if (!btn)
                return;
            buttons_[static_cast<uint8_t>(type)] = btn;
        }
        void Levoit::publish_switch(SwitchType type, bool state)
        {
            if (type == SwitchType::DISPLAY)
                display_on_ = state;
#ifdef USE_SWITCH
            auto *sw = switches_[st_idx_(type)];
            if (!sw)
                return;
            // Mark the entity as "has state" BEFORE the dedup early-return below,
            // so a decoder publish that matches the entity's default-initialized
            // value (e.g. boot decode of qc_enabled=0 against the default sw->state=false)
            // still flips has_state_ from false to true. ESPHome's Switch::publish_state
            // doesn't do this itself (unlike Select/Number); see comment in
            // LevoitSwitch::write_state for the rationale.
            sw->set_has_state(true);
            if (sw->state == state)
                return;
            sw->publish_state(state);
#endif
        }

        void Levoit::publish_sensor(SensorType type, float value)
        {
#ifdef USE_SENSOR
            auto *se = sensors_[st_idx_(type)];
            if (!se)
                return;
            if (se->has_state() && se->state == value)
                return;
            se->publish_state(value);
#endif
        }

        void Levoit::publish_number(NumberType type, float value)
        {
#ifdef USE_NUMBER
            auto *nm = numbers_[nt_idx_(type)];
            if (!nm)
                return;
            if (nm->has_state() && nm->state == value)
                return;
            nm->publish_state(value);
#endif
        }
        void Levoit::publish_select(SelectType type, uint32_t value)
        {
#ifdef USE_SELECT
            auto *sl = selects_[sl_idx_(type)];
            if (!sl)
                return;
            const auto &options = sl->traits.get_options();
            if (type == SelectType::FAN_OPERATING_MODE_SELECT)
            {
                const char *opt = fan_operating_mode_to_option_(value);
                if (opt == nullptr)
                {
                    ESP_LOGW(TAG, "publish_select: invalid fan mode %u", (unsigned)value);
                    return;
                }
                if (std::find(options.begin(), options.end(), opt) == options.end())
                {
                    ESP_LOGW(TAG, "publish_select: unsupported fan mode '%s' for this model", opt);
                    return;
                }
                if (sl->has_state() && sl->current_option() == opt)
                    return;
                sl->publish_state(opt);
                return;
            }
            if (value >= options.size())
            {
                ESP_LOGW(TAG, "publish_select: invalid index %u for type %d (options=%u)",
                         (unsigned)value, (int)type, (unsigned)options.size());
                return;
            }
            const std::string &opt = options[value];
            if (sl->has_state() && sl->current_option() == opt)
                return;
            sl->publish_state(opt);
#endif
        }
        void Levoit::publish_text_sensor(TextSensorType type, const std::string &value)
        {
#ifdef USE_TEXT_SENSOR
            auto *tsl = text_sensor_[static_cast<uint8_t>(type)];
            if (!tsl)
                return;
            if (tsl->has_state() && tsl->state == value)
                return;
            tsl->publish_state(value);
#endif
        }
        void Levoit::publish_binary_sensor(BinarySensorType type, bool state)
        {
            // Store the desired state; platform entity will publish from its loop
            binary_sensor_states_[bs_idx_(type)] = state;
        }

        void Levoit::update_bulk_pref(uint8_t tlv_id, uint32_t value)
        {
            // Maps status TLV id (0x18..0x23) to BulkPrefsCache field +
            // seen_mask bit. The seen_mask uses (tlv_id - 0x18) as bit
            // position so the 12 TLVs map linearly to bits 0..11.
            if (tlv_id < 0x18 || tlv_id > 0x23) {
                ESP_LOGW("levoit.bulk_prefs", "update_bulk_pref: out-of-range tlv_id=0x%02X", tlv_id);
                return;
            }
            const bool was_valid = bulk_prefs_.valid();
            switch (tlv_id) {
                case 0x18: bulk_prefs_.sleep_type = (uint8_t)value; break;
                case 0x19: bulk_prefs_.qc_enabled = (uint8_t)value; break;
                case 0x1A: bulk_prefs_.qc_min    = (uint16_t)value; break;
                case 0x1B: bulk_prefs_.qc_fan    = (uint8_t)value; break;
                case 0x1C: bulk_prefs_.wn_enabled= (uint8_t)value; break;
                case 0x1D: bulk_prefs_.wn_min    = (uint16_t)value; break;
                case 0x1E: bulk_prefs_.wn_fan    = (uint8_t)value; break;
                case 0x1F: bulk_prefs_.sleep_fan = (uint8_t)value; break;
                case 0x20: bulk_prefs_.sleep_min = (uint16_t)value; break;
                case 0x21: bulk_prefs_.dt_enabled= (uint8_t)value; break;
                case 0x22: bulk_prefs_.dt_mode   = (uint8_t)value; break;
                case 0x23: bulk_prefs_.dt_level  = (uint8_t)value; break;
            }
            bulk_prefs_.seen_mask |= (uint16_t)(1u << (tlv_id - 0x18));
            ESP_LOGD("levoit.bulk_prefs",
                     "update tlv=0x%02X val=%u seen_mask=0x%03X",
                     tlv_id, (unsigned)value, (unsigned)bulk_prefs_.seen_mask);
            if (!was_valid && bulk_prefs_.valid()) {
                ESP_LOGI("levoit.bulk_prefs",
                         "cache now VALID — sleep[type=%u fan=%u min=%u] "
                         "QC[en=%u fan=%u min=%u] WN[en=%u fan=%u min=%u] "
                         "DT[en=%u mode=%u lvl=%u]",
                         bulk_prefs_.sleep_type, bulk_prefs_.sleep_fan, bulk_prefs_.sleep_min,
                         bulk_prefs_.qc_enabled, bulk_prefs_.qc_fan, bulk_prefs_.qc_min,
                         bulk_prefs_.wn_enabled, bulk_prefs_.wn_fan, bulk_prefs_.wn_min,
                         bulk_prefs_.dt_enabled, bulk_prefs_.dt_mode, bulk_prefs_.dt_level);
            }
        }
#ifdef USE_LIGHT
        void Levoit::publish_sprout_light(bool on, float brightness, float color_temp, bool breathing)
        {
            if (sprout_light_ != nullptr)
                sprout_light_->apply_from_mcu(on, brightness, color_temp, breathing);
        }
#else
        void Levoit::publish_sprout_light(bool on, float brightness, float color_temp, bool breathing) {}
#endif

        void Levoit::sendSproutLightDirect(bool on, bool breathing, uint8_t bri_pct, uint16_t ct_k)
        {
            // Store values so build_sprout_command reads them instead of number states
            this->pending_led_bri_ = bri_pct;
            this->pending_led_ct_ = ct_k;
            if (!on) {
                this->sendCommand(setSproutLedOff);
            } else if (breathing) {
                this->sendCommand(setSproutLightBreathing);
            } else {
                this->sendCommand(setSproutLightNightlight);
            }
        }

        void Levoit::send_aqi_to_mcu(uint16_t aqi)
        {
            if (!this->display_on_)
                return;
            this->pending_aqi_ = aqi;
            this->sendCommand(setSproutAqiScale);
        }

        void Levoit::publish_filter_stats_now()
        {
            float filter_left = this->calculate_filter_life_left_percent();
#ifdef USE_SENSOR
            auto *se = this->sensors_[st_idx_(SensorType::FILTER_LIFE_LEFT)];
            if (se != nullptr)
                se->publish_state(filter_left);
#endif
            this->publish_binary_sensor(BinarySensorType::FILTER_LOW, filter_left < 5.0f);
            this->push_filter_pct_if_changed(filter_left);
        }

        // EverestAir: mirror the ESP-computed filter % onto the MCU's panel
        // indicator via CMD=02 05 55. Only sends when the rounded integer %
        // changes, so the periodic recompute doesn't spam the MCU.
        void Levoit::push_filter_pct_if_changed(float filter_left)
        {
            if (this->model_ != ModelType::EVERESTAIR)
                return;
            int pct = static_cast<int>(filter_left + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            if (pct == this->last_sent_filter_pct_)
                return;
            this->last_sent_filter_pct_ = pct;
            this->set_pending_filter_pct(static_cast<uint8_t>(pct));
            this->sendCommand(CommandType::setFilterPercent);
            ESP_LOGD(TAG, "Pushed filter %% to MCU panel: %d%%", pct);
        }
        void Levoit::on_switch_command(SwitchType type, bool state)
        {
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == SwitchType::QUICK_CLEAN) return;

            switch (type)
            {
            case SwitchType::DISPLAY:
                this->sendCommand(state ? setDisplayOn : setDisplayOff);
                break;

            case SwitchType::CHILD_LOCK:
                this->sendCommand(state ? setDisplayLockOn : setDisplayLockOff);
                break;

            case SwitchType::LIGHT_DETECT:
                this->sendCommand(state ? setLightDetectOn : setLightDetectOff);
                break;

            case SwitchType::QUICK_CLEAN:
            case SwitchType::DAYTIME_ENABLED:
                // Part of the 12-TLV bulk write — see setBulkPrefs. The new
                // switch state has already been optimistically published by
                // LevoitSwitch::write_state before this handler runs (and
                // set_has_state(true) is now called explicitly there — see
                // e567c31), so the builder reads it via get_switch(...)->state.
                this->sendCommand(setBulkPrefs);
                break;

            case SwitchType::WHITE_NOISE:
                this->sendCommand(state ? setSproutWhiteNoiseOn : setSproutWhiteNoiseOff);
                break;

            case SwitchType::LED_RING:
                this->sendCommand(state ? setSproutLightNightlight : setSproutLedOff);
                break;

            case SwitchType::AUTO_DRY_POWER_OFF:
                this->sendCommand(state ? setAutoDryPowerOffOn : setAutoDryPowerOffOff);
                break;

            case SwitchType::AUTO_DRY_WATER_EMPTY:
                this->sendCommand(state ? setAutoDryWaterEmptyOn : setAutoDryWaterEmptyOff);
                break;

            default:
                break;
            }
        }

        void Levoit::on_number_command(NumberType type, float value)
        {
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == NumberType::EFFICIENCY_ROOM_SIZE) return;

            switch (type)
            {
            case NumberType::TIMER:
                if (this->model_ == ModelType::SUPERIOR6000S) {
                    // Superior 6000S: the number is in HOURS and the ESP runs the
                    // countdown - the MCU only stores the remaining value we push.
                    uint32_t secs = static_cast<uint32_t>(value * 3600);
                    this->sendCommand(setTimerMinutes);
                    if (secs > 0) {
                        this->start_esp_timer(secs);
                        uint16_t mins = secs / 60;
                        this->publish_text_sensor(TextSensorType::TIMER_DURATION_INITIAL,
                                                  format_duration_minutes(mins));
                        this->publish_sensor(SensorType::TIMER_CURRENT, value);
                        this->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT,
                                                  format_duration_minutes(mins));
                    } else {
                        this->stop_esp_timer();
                        this->publish_sensor(SensorType::TIMER_CURRENT, 0.0f);
                        this->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT,
                                                  format_duration_minutes(0));
                    }
                } else if (this->model_ == ModelType::CORE200S) {
                    // Core200S: MCU requires stop before accepting new timer value;
                    // after stop, poll requestTimerStatus until MCU confirms remaining=0
                    if (value == 0) {
                        this->sendCommand(setTimerStop);
                        this->set_timer_stop_pending(true);
                    } else if (this->is_timer_active()) {
                        this->sendCommand(setTimerStop);
                        this->set_timeout(500, [this]() {
                            this->sendCommand(setTimerMinutes);
                        });
                    } else {
                        this->sendCommand(setTimerMinutes);
                    }
                } else {
                    // Other models: direct stop or set
                    this->sendCommand(value == 0 ? setTimerStop : setTimerMinutes);
                }
                break;

            case NumberType::EFFICIENCY_ROOM_SIZE:
                this->sendCommand(setAutoModeEfficient); // takes value from number: Room Size
                if (supports_auto_fan_mode_(this->model_))
                    this->sendCommand(setFanModeAuto);
                break;

            case NumberType::AUTO_PROFILE_ROOM_SIZE_INPUT:
                // Just save — the MCU command fires when Room Size/Efficient
                // is selected via the Auto Mode select, not on every edit.
                break;

            case NumberType::SLEEP_MODE_MIN:
            case NumberType::SLEEP_FAN_LEVEL:
            case NumberType::QUICK_CLEAN_MIN:
            case NumberType::QUICK_CLEAN_FAN_LEVEL:
            case NumberType::DAYTIME_FAN_LEVEL:
                // Bulk-prefs cluster fields — all route through the same 12-TLV
                // write at CMD 02 02 55 tags 0x04..0x0F. The builder reads the
                // new value via get_number(...)->state (optimistically published
                // by LevoitNumber::control before this handler fires) and the
                // rest from bulk_prefs_ cache. Sleep_type byte must be non-zero
                // (Custom1/Custom2) for writes to non-type fields to apply —
                // see docs/STOCK_FIRMWARE_FINDINGS.md "Gate behavior".
                this->sendCommand(setBulkPrefs);
                break;

            case NumberType::LED_BRIGHTNESS_MIN:
            case NumberType::LED_SPEED:
                this->sendCommand(setSproutLightBreathing);
                break;

            case NumberType::WHITE_NOISE_VOLUME:
                this->sendCommand(setSproutWhiteNoiseOn);
                break;

            case NumberType::AQI_SCALE:
                this->sendCommand(setSproutAqiScale);
                break;

            case NumberType::VENT_ANGLE:
                this->sendCommand(setVentAngle);
                break;

            case NumberType::HUMIDITY_TARGET:
                this->sendCommand(setHumidityTarget);
                break;
            }
        }
        void Levoit::on_select_command(SelectType type, uint32_t value)
        {
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == SwitchType::QUICK_CLEAN) return;

            switch (type)
            {
            case SelectType::AUTO_PROFILE:
                this->sendCommand(value == 1 ? setAutoProfileAway : setAutoProfileHome);
                break;

            case SelectType::HUMIDITY_SUBTYPE:
                this->sendCommand(value == 1 ? setHumiditySubtypeFan : setHumiditySubtypeSmart);
                break;

            case SelectType::DRY_LEVEL:
                // Remembered only. Drying is started from the fan entity's Dry mode,
                // which then applies this level - the MCU has no standalone dry-level command.
                this->dry_level_preference_ = (value <= 1) ? (uint8_t) value : 0;
                break;

            case SelectType::FAN_OPERATING_MODE_SELECT:
                this->on_fan_command(-1, -1, value);
                break;

            case SelectType::AUTO_MODE:
            {
                bool sent_auto_profile = false;
                if (this->model_ == ModelType::EVERESTAIR)
                {
                    // EverestAir options are {"Default"(idx0→mode0), "Eco"(idx1→mode3)}
                    this->sendCommand(value == 1 ? setAutoModeEco : setAutoModeDefault);
                    sent_auto_profile = true;
                }
                else
                {
                    switch (value)
                    {
                    case 0:
                        this->sendCommand(setAutoModeDefault);
                        sent_auto_profile = true;
                        break;
                    case 1:
                        this->sendCommand(setAutoModeQuiet);
                        sent_auto_profile = true;
                        break;
                    case 2:
                        // Inject the remembered room size target so the command
                        // builder sends the right value, not the last MCU-echoed one.
                        {
                            auto *input = this->get_number(NumberType::AUTO_PROFILE_ROOM_SIZE_INPUT);
                            auto *eff   = this->get_number(NumberType::EFFICIENCY_ROOM_SIZE);
                            if (input != nullptr && eff != nullptr && input->has_state())
                                eff->publish_state(input->state);
                        }
                        this->sendCommand(setAutoModeEfficient);
                        sent_auto_profile = true;
                        break;
                    case 3:
                        this->sendCommand(setAutoModeEco);
                        sent_auto_profile = true;
                        break;
                    default:
                        break;
                    }
                }
                if (sent_auto_profile && supports_auto_fan_mode_(this->model_))
                    this->sendCommand(setFanModeAuto);
                break;
            }

            case SelectType::SLEEP_MODE:
                switch (value)
                {
                case 0:
                    this->sendCommand(setSleepModeDefault);
                    break;
                case 1:
                    // this->sendCommand(setSleepModeCustom);
                    break;
                default:
                    break;
                }
                break;

            case SelectType::SLEEP_PREFERENCE:
            case SelectType::DAYTIME_FAN_MODE:
                // Bulk-prefs SET: SLEEP_PREFERENCE is the gate byte (TLV 0x18);
                // DAYTIME_FAN_MODE is the daytime preset's fan-mode enum
                // (TLV 0x22). The builder reads the new option index via
                // active_index() on the optimistically-published select. For
                // SLEEP_PREFERENCE, value 0 (Default) locks tags 0x05..0x0F;
                // values 1/2 (Custom1/Custom2) unlock writes to the rest of
                // the cluster (see docs/STOCK_FIRMWARE_FINDINGS.md "Gate
                // behavior"). DAYTIME_FAN_MODE is a non-gate field that
                // requires sleep_type ≠ 0 to apply.
                this->sendCommand(setBulkPrefs);
                break;

            case SelectType::NIGHTLIGHT:
                switch (value)
                {
                case 0:
                    this->sendCommand(setNightlightOff);
                    break;
                case 1:
                    this->sendCommand(setNightlightMid);
                    break;
                case 2:
                    this->sendCommand(setNightlightFull);
                    break;
                default:
                    break;
                }
                break;

            case SelectType::LIGHT_MODE:
                // 0=Off, 1=Nightlight, 2=Breathing
                switch (value)
                {
                case 0:
                    this->sendCommand(setSproutLedOff);
                    break;
                case 1:
                    this->sendCommand(setSproutLightNightlight);
                    break;
                case 2:
                    this->sendCommand(setSproutLightBreathing);
                    break;
                default:
                    break;
                }
                break;

            case SelectType::WHITE_NOISE_SOUND:
                // Sound changed — re-send WN on with new sound index
                this->sendCommand(setSproutWhiteNoiseOn);
                break;

            default:
                break;
            }
        }
        void Levoit::on_fan_command(int power, int speed_level, int mode)
        {
            ESP_LOGD(TAG, "on_fan_command: power=%d speed_level=%d mode=%d", power, speed_level, mode);
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == SwitchType::QUICK_CLEAN) return;

            if (power != -1)
            {
                this->sendCommand(power == 1 ? setDeviceON : setDeviceOFF);
            }
            if (speed_level != -1)
            {
                switch (speed_level)
                {
                case 1:
                    this->sendCommand(setDeviceFanLvl1);
                    break;
                case 2:
                    this->sendCommand(setDeviceFanLvl2);
                    break;
                case 3:
                    this->sendCommand(setDeviceFanLvl3);
                    break;
                case 4:
                    this->sendCommand(setDeviceFanLvl4);
                    break;
                case 5:
                    this->sendCommand(setDeviceFanLvl5);
                    break;
                case 6:
                    this->sendCommand(setDeviceFanLvl6);
                    break;
                case 7:
                    this->sendCommand(setDeviceFanLvl7);
                    break;
                case 8:
                    this->sendCommand(setDeviceFanLvl8);
                    break;
                case 9:
                    this->sendCommand(setDeviceFanLvl9);
                    break;
                default:
                    break;
                }
            }
            if (mode != -1)
            {
                switch (mode)
                {
                case 0:
                    this->sendCommand(setFanModeManual);
                    break;
                case 1:
                    this->sendCommand(setFanModeSleep);
                    break;
                case 2:
                    this->sendCommand(setFanModeAuto);
                    break;
                case 4:
                    this->sendCommand(setFanModeTurbo);
                    break;
                case 3:
                    this->sendCommand(setFanModeHumidity);
                    break;
                case 5:
                    this->sendCommand(setFanModePet);
                    break;
                case 6:
                    // Superior 6000S Dry mode - level comes from the DRY_LEVEL select
                    this->sendCommand(this->dry_level_preference_ == 1 ? setDryLevelHigh : setDryLevelLow);
                    break;
                default:
                    break;
                }
            }
        }

        void Levoit::dump_config() {}

        void Levoit::set_device_model(std::string model)
        {
            if (model == "VITAL200S")
                model_ = ModelType::VITAL200S;
            else if (model == "VITAL100S")
                model_ = ModelType::VITAL100S;
            else if (model == "CORE200S")
                model_ = ModelType::CORE200S;
            else if (model == "CORE300S")
                model_ = ModelType::CORE300S;
            else if (model == "CORE400S")
                model_ = ModelType::CORE400S;
            else if (model == "CORE600S")
                model_ = ModelType::CORE600S;
            else if (model == "SPROUT")
                model_ = ModelType::SPROUT;
            else if (model == "EVERESTAIR")
                model_ = ModelType::EVERESTAIR;
            else if (model == "SUPERIOR6000S")
                model_ = ModelType::SUPERIOR6000S;

            ESP_LOGI(TAG, "Model set to: %s (ModelType=%d)", model.c_str(), (int)model_);
        }
        void Levoit::setup()
        {
            ESP_LOGI(TAG, "Setting up Levoit %s", model_ == ModelType::VITAL200S ? "VITAL200S" : "VITAL100S");
            //https://docs.google.com/spreadsheets/d/17j6FZwvqHRFkGoH5996u5JdR7tk4_7fNuTxAK7kc4Fk/edit?gid=1612245341#gid=1612245341
            if (model_ == ModelType::VITAL200S)
                cadr = 415;
            if (model_ == ModelType::VITAL100S)
                cadr = 221;
            if (model_ == ModelType::CORE300S)
                cadr = 214; 
            if (model_ == ModelType::CORE400S)
                cadr = 442;
            if (model_ == ModelType::CORE200S)
                cadr = 167;
            if (model_ == ModelType::CORE600S)
                cadr = 641;
            if (model_ == ModelType::SPROUT)
                cadr = 145; // spec smoke CADR 144.5 m³/h (rounded)
            if (model_ == ModelType::EVERESTAIR)
                cadr = 612;
            if (model_ == ModelType::SUPERIOR6000S)
                cadr = 500;   // humidifier: nominal, only feeds the CADR counters
            
            // Initialize preferences for tracking used_cadr and total_runtime
            pref_used_cadr_ = global_preferences->make_preference<uint32_t>(fnv1_hash("levoit_used_cadr"));
            pref_total_runtime_ = global_preferences->make_preference<uint32_t>(fnv1_hash("levoit_runtime"));
            
            // Restore saved values or initialize to 0
            if (pref_used_cadr_.load(&used_cadr_)) {
                ESP_LOGI(TAG, "Restored used_cadr: %u m³", (unsigned)used_cadr_);
            } else {
                used_cadr_ = 0;
                ESP_LOGI(TAG, "Initialized used_cadr to 0");
            }
            if (pref_total_runtime_.load(&total_runtime_)) {
                ESP_LOGI(TAG, "Restored total_runtime: %u min", (unsigned)total_runtime_);
            } else {
                total_runtime_ = 0;
                ESP_LOGI(TAG, "Initialized total_runtime to 0");
            }
                               
            // Set LED to blink on initial connect until WiFi is connected.
            // Delay slightly so the MCU has time to finish booting before we send commands.
            this->set_timeout("wifi_led_init", 2000, [this]() {
                this->sendCommand(CommandType::setWifiLedBlinking);
            });
            this->sendCommand(CommandType::setFilterLedOn);
            filter_led_on_ = true;
            filter_blinking_ = true;

            // EverestAir: push the computed filter % to the MCU panel on boot
            // (delay so the MCU has finished booting), then keep it in sync
            // whenever the value changes (see push_filter_pct_if_changed).
            if (this->model_ == ModelType::EVERESTAIR) {
                this->set_timeout("everest_filter_init", 3000, [this]() {
                    this->push_filter_pct_if_changed(this->calculate_filter_life_left_percent());
                });
            }

            // Track CADR on initial setup
            track_cadr_usage();
        }
        
        void Levoit::track_cadr_usage()
        {
            // Get fan state - check if fan is ON using .state member
            if (this->fan_ != nullptr && this->fan_->state) {
                // Fan is enabled - track usage
                total_runtime_++;
                
                // Get fan speed level (1-4) from .speed member
                int speed = this->fan_->speed;
                if (speed > 0 && speed <= 4) {
                    // Use helper to compute current CADR/hour, then convert to per-minute
                    uint32_t cadr_per_hour = this->calculate_current_cadr_per_hour();
                    uint32_t cadr_per_min = cadr_per_hour / 60;
                    used_cadr_ += cadr_per_min;
                    ESP_LOGD(TAG, "CADR tracked: +%u m³ (speed=%d, total=%u m³, runtime=%u min)", 
                             (unsigned)cadr_per_min, speed, (unsigned)used_cadr_,
                             (unsigned)total_runtime_);
                    
                }
                
                // Calculate and publish filter life left (once per minute here)
                float filter_left = this->calculate_filter_life_left_percent();
#ifdef USE_SENSOR
                auto *se = this->sensors_[st_idx_(SensorType::FILTER_LIFE_LEFT)];
                if (se != nullptr)
                    se->publish_state(filter_left);
#endif
                this->push_filter_pct_if_changed(filter_left);

                // Save to preferences every minute when running
                pref_used_cadr_.save(&used_cadr_);
                pref_total_runtime_.save(&total_runtime_);
            }
        }

        uint32_t Levoit::calculate_current_cadr_per_hour() const
        {
            if (this->fan_ == nullptr || !this->fan_->state)
                return 0;
            int speed = this->fan_->speed;

            // EverestAir: 3 manual levels at 1/4, 1/2, 3/4 of full CADR + Turbo = full.
            // Turbo reports FanLevel 4 but the fan clamps speed to 3 (speed_count_=3),
            // so it can't be told apart from level 3 by speed — detect it via preset.
            if (this->model_ == ModelType::EVERESTAIR)
            {
                esphome::StringRef preset = this->fan_->get_preset_mode();
                if (!preset.empty() && preset == "Turbo")
                    return cadr;  // full CADR
                if (speed >= 1 && speed <= 3)
                    return (cadr * (uint32_t)speed) / 4u;  // 1/4, 1/2, 3/4
                return 0;
            }

            // Determine max speed based on model (Core300S has 3 speeds)
            uint32_t max_speed = (this->model_ == ModelType::CORE300S)     ? 3u
                                 : (this->model_ == ModelType::SUPERIOR6000S) ? 9u
                                                                              : 4u;
            if (speed <= 0 || (uint32_t)speed > max_speed)
                return 0;
            uint32_t result = (cadr * (uint32_t)speed) / max_speed;

            // Sleep mode derates level 1 to 63%
            esphome::StringRef preset = this->fan_->get_preset_mode();
            if (!preset.empty() && preset == "Sleep" && speed == 1)
            {
                result = (uint32_t)(result * 0.63f);
            }

            return result;
        }

        float Levoit::calculate_filter_life_left_percent() const
        {
#ifndef USE_NUMBER
            return 100.0f;
#else
            auto *filter_lifetime_num = this->get_number(NumberType::FILTER_LIFETIME_MONTHS);
            if (filter_lifetime_num == nullptr || !filter_lifetime_num->has_state())
                return 100.0f;

            float filter_lifetime_months = filter_lifetime_num->state;
            uint32_t total_filter_capacity = cadr * 24 * 30 * filter_lifetime_months;
            if (total_filter_capacity == 0)
                return 100.0f;

            float life_left_percent = 100.0f - ((float)used_cadr_ / (float)total_filter_capacity * 100.0f);
            if (life_left_percent < 0.0f)
                life_left_percent = 0.0f;
            if (life_left_percent > 100.0f)
                life_left_percent = 100.0f;

            return life_left_percent;
#endif
        }

        /// @brief The main loop, that is triggeed by the esphome framework automatically
        void Levoit::loop()
        {
            static uint32_t last_check, last_check_sec = 0;
            static uint32_t last_check_min = 30000;
            static uint32_t last_cadr_check = 0;
            static uint32_t last_filter_check = 0;
            uint32_t now = millis();
            
            // Every minute: track CADR usage and runtime
            if (now - last_check_min >= 60000)
            {
                last_check_min = now;
                track_cadr_usage();
            }
            
            if (now - last_check_sec >= 1000)
            {
                // every second
                last_check_sec = now;
                // Handle filter LED blinking
                if (filter_blinking_)
                {
                    // toggle filter led
                    if (filter_led_on_)
                    {
                        this->sendCommand(CommandType::setFilterLedOff);
                        filter_led_on_ = false;
                    }
                    else
                    {
                        this->sendCommand(CommandType::setFilterLedOn);
                        filter_led_on_ = true;
                    }
                }
                // Check WiFi status and update LED accordingly
                auto *wifi = wifi::global_wifi_component;
                if (wifi != nullptr)
                {
                    bool is_connected = wifi->is_connected();
                    bool is_disabled = wifi->is_disabled();
                    // Determine WiFi state: connecting = not connected and not disabled
                    bool is_connecting = !is_connected && !is_disabled;

                    if (is_connected && !wifi_led_solid_)
                    {
                        ESP_LOGD(TAG, "WiFi connected - setting LED solid");
                        wifi_led_solid_ = true;
                        wifi_led_blink_sent_ = false;
                        this->sendCommand(CommandType::setFilterLedOff);
                        filter_led_on_ = false;
                        filter_blinking_ = false;
                        // Ensure any blinking is cleared before setting solid LED
                        this->sendCommand(CommandType::setWifiLedOff);
                        if (this->model_ == ModelType::CORE200S)
                        {
                            this->set_timeout(500, [this]() {
                                this->sendCommand(CommandType::setWifiLedOn);
                                this->set_timeout(500, [this]() {
                                    this->sendCommand(CommandType::setWifiLedOn);
                                });
                            });
                        }
                        else
                        {
                            this->sendCommand(CommandType::setWifiLedOn);
                        }
                    }
                    else if (is_connecting && wifi_led_solid_)
                    {
                        ESP_LOGD(TAG, "WiFi connecting - setting LED blinking");
                        this->sendCommand(CommandType::setWifiLedBlinking);
                        wifi_led_solid_ = false;
                    }
                    else if (is_connecting && !wifi_led_solid_ && !wifi_led_blink_sent_)
                    {
                        // Retry blinking on initial boot in case the setup() command was missed
                        ESP_LOGD(TAG, "WiFi connecting - initial blink retry");
                        this->sendCommand(CommandType::setWifiLedBlinking);
                        wifi_led_blink_sent_ = true;
                    }
                    else if (is_disabled && wifi_led_solid_)
                    {
                        ESP_LOGD(TAG, "WiFi disabled - setting LED off");
                        this->sendCommand(CommandType::setWifiLedOff);
                        wifi_led_solid_ = false;
                    }
                }else {
                    ESP_LOGW(TAG, "WiFi component not found - cannot update LED status");       
                }
            }

            // Every 5 seconds: compute and publish current CADR/hour
            if (now - last_cadr_check >= 5000)
            {
                last_cadr_check = now;
                uint32_t current_cadr_hour = this->calculate_current_cadr_per_hour();
                this->publish_sensor(SensorType::CURRENT_CADR, current_cadr_hour);
            }

            // Every 10 seconds: publish filter life left
            if (now - last_filter_check >= 10000)
            {
                last_filter_check = now;
                float filter_left = this->calculate_filter_life_left_percent();
#ifdef USE_SENSOR
                auto *se = this->sensors_[st_idx_(SensorType::FILTER_LIFE_LEFT)];
                if (se != nullptr)
                    se->publish_state(filter_left);
#endif
                this->publish_binary_sensor(BinarySensorType::FILTER_LOW, filter_left < 5.0f);
                this->push_filter_pct_if_changed(filter_left);
            }

            if (this->model_ == ModelType::SUPERIOR6000S)
            {
                // Superior 6000S: the MCU does not count down on its own - it only
                // accepts a 'remaining' value pushed to it - so the ESP owns the
                // timer and refreshes the MCU once a minute.
                if (this->esp_timer_active_ && now - esp_timer_last_update_ >= 60000)
                {
                    esp_timer_last_update_ = now;
                    uint32_t elapsed_secs = (now - esp_timer_start_millis_) / 1000;
                    uint32_t remaining = (elapsed_secs >= esp_timer_duration_secs_)
                                             ? 0
                                             : (esp_timer_duration_secs_ - elapsed_secs);
                    uint16_t remaining_min = remaining / 60;
                    float remaining_hours = remaining / 3600.0f;

                    if (remaining > 0)
                    {
                        ESP_LOGD(TAG, "ESP timer update: %u sec remaining", remaining);
                        this->send_timer_update(remaining);
                        this->publish_sensor(SensorType::TIMER_CURRENT, remaining_hours);
                        this->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT,
                                                  format_duration_minutes(remaining_min));
                    }
                    else if (esp_timer_zero_count_ < 5)
                    {
                        // Repeat the zero a few times: a single one is sometimes missed
                        // and the panel would keep showing a running timer.
                        ESP_LOGD(TAG, "ESP timer expired, sending zero command %u/5",
                                 esp_timer_zero_count_ + 1);
                        this->send_timer_update(0);
                        esp_timer_zero_count_++;
                    }
                    else
                    {
                        ESP_LOGI(TAG, "ESP timer finished, turning off device");
                        this->stop_esp_timer();
                        this->publish_number(NumberType::TIMER, 0.0f);
                        this->publish_sensor(SensorType::TIMER_CURRENT, 0.0f);
                        this->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT,
                                                  format_duration_minutes(0));
                        this->sendCommand(setDeviceOFF);
                    }
                }
            }
            else if (this->timer_active_ && now - last_check >= 10000)
            { // 10 seconds
                // timer active?
                last_check = now;
                ESP_LOGD(TAG, "Request status - timer");
                this->sendCommand(requestTimerStatus);
            }

            // Poll timer status every 1s after sending stop, until MCU confirms remaining=0
            if (this->timer_stop_pending_ && now - this->timer_stop_sent_at_ >= 1000)
            {
                this->timer_stop_sent_at_ = now;
                ESP_LOGD(TAG, "Polling timer status (stop pending)");
                this->sendCommand(requestTimerStatus);
            }
            // 1) Read incoming bytes into buffer_
            while (available())
            {
                uint8_t b;
                read_byte(&b);

                if (buf_len_ < sizeof(buffer_))
                {
                    buffer_[buf_len_++] = b;
                }
                else
                {
                    ESP_LOGW(TAG, "RX buffer overflow -> reset");
                    buf_len_ = 0;
                    return;
                }
            }

            // 2) Parse as many frames as possible from the buffer
            while (true)
            {
                if (buf_len_ < 1)
                    break;

                // Find 0xA5
                size_t start = 0;
                while (start < buf_len_ && buffer_[start] != 0xA5)
                    start++;

                if (start == buf_len_)
                { // no start
                    buf_len_ = 0;
                    break;
                }

                // Drop junk before start
                if (start > 0)
                {
                    memmove(buffer_, buffer_ + start, buf_len_ - start);
                    buf_len_ -= start;
                }

                // Need at least 6 bytes (up to checksum byte)
                if (buf_len_ < 6)
                    break;

                const uint8_t size_after_checksum = buffer_[3];
                const size_t expected_len = 6 + size_after_checksum;

                // sanity guard (avoid insane lengths)
                if (expected_len > sizeof(buffer_))
                {
                    ESP_LOGW(TAG, "RX length too big (%u) -> resync", (unsigned)expected_len);
                    memmove(buffer_, buffer_ + 1, buf_len_ - 1);
                    buf_len_ -= 1;
                    continue;
                }

                if (buf_len_ < expected_len)
                    break; // wait more bytes

                // Validate checksum over the WHOLE frame length, excluding byte[5]
                const uint8_t expected_chk = levoit_checksum(buffer_, expected_len);
                const uint8_t got_chk = buffer_[5];

                if (got_chk != expected_chk)
                {
                    ESP_LOGW(TAG, "Bad checksum: got=0x%02X expected=0x%02X; resync", got_chk, expected_chk);
                    // resync: drop start byte and try again
                    memmove(buffer_, buffer_ + 1, buf_len_ - 1);
                    buf_len_ -= 1;
                    continue;
                }

                // Good frame -> you can now split header/payload
                // bytes 0..9 are your "header fields" (if size allows)
                const uint8_t *frame = buffer_;
                const size_t frame_len = expected_len;

                // after-checksum block begins at byte 6
                const uint8_t *after_checksum = frame + 6;
                const size_t after_checksum_len = frame_len - 6;

                // if you want "payload after your 10-byte header"
                const uint8_t *payload = nullptr;
                size_t payload_len = 0;
                if (frame_len >= 10)
                {
                    payload = frame + 10;
                    payload_len = frame_len - 10;
                }

                // Debug log frame
                char hexbuf[1024];
                size_t pos = 0;
                for (size_t i = 0; i < frame_len && pos < sizeof(hexbuf) - 6; i++)
                {
                    pos += snprintf(hexbuf + pos, sizeof(hexbuf) - pos, "0x%02X ", frame[i]);
                }

                ESP_LOGV(TAG, "<<< RX packet (%u bytes): %s", (unsigned)frame_len, hexbuf);

                // Example: log key header bytes if present
                if (frame_len >= 10)
                {
                    ESP_LOGD(TAG,
                             "<<< HEADER: ptype=0x%02X 0x%02X | type=0x%02X ",
                             frame[7], frame[8], frame[1]);
                }
                if (frame_len > 10)
                {
                    const uint8_t *payload = frame + 10;
                    size_t payload_len = frame_len - 10;

                    char phex[512];
                    size_t pos = 0;

                    for (size_t i = 0; i < payload_len && pos < sizeof(phex) - 6; i++)
                    {
                        pos += snprintf(phex + pos, sizeof(phex) - pos, "0x%02X ", payload[i]);
                    }

                    ESP_LOGD(TAG, "<<< PAYLOAD Size: %u bytes", (unsigned)payload_len);
                    ESP_LOGV(TAG, "<<< PAYLOAD: %s", phex);
                }
                else
                {
                    ESP_LOGD(TAG, "<<< PAYLOAD Size: 0 bytes");
                }

                // Process
                process_message((uint8_t *)frame, (int)frame_len);

                // Remove processed packet
                memmove(buffer_, buffer_ + frame_len, buf_len_ - frame_len);
                buf_len_ -= frame_len;
            }

            delay(1);
        }

        /// @brief Processes all messages that are coming from the air purifier secondary chip on UART1
        /// @param msg
        /// @param len
        void Levoit::process_message(std::uint8_t *msg, int len)
        {
            if (len < 10)
                return;
            if (msg[0] != 0xA5)
                return;

            const uint8_t msg_type = msg[1];
            const uint8_t seq = msg[2];
            const uint8_t size_af = msg[3];
            const uint8_t cmd_sz = msg[6];
            const uint8_t ptype0 = msg[7];
            const uint8_t ptype1 = msg[8];

            // Frame sanity
            ESP_LOGD(TAG,
                     "RX: type=0x%02X seq=%u size_after_chk=%u cmdSz=%u ptype=%02X%02X model=%d",
                     msg_type, seq, size_af, cmd_sz, ptype0, ptype1, (int)model_);

            // Payload starts after the 10-byte header
            const uint8_t *payload = nullptr;
            size_t payload_len = 0;

            if (len > 10)
            {
                payload = msg + 10;
                payload_len = len - 10;
            }

            ESP_LOGD(TAG, ">>> Calling dispatch_decoder with model=%d ptype=%02X%02X", (int)model_, ptype0, ptype1);
            dispatch_decoder(this, model_, msg_type, seq, cmd_sz, ptype0, ptype1, payload, payload_len);
        }

        /// @brief Sends the commands depending on its type. Delegates to model-specific command builders.
        /// @param commandType
        void Levoit::sendCommand(CommandType commandType)
        {
            ESP_LOGD(TAG, "Command triggered: %s", command_type_to_string(commandType));
            std::vector<uint8_t> message;

            if (this->model_ == ModelType::CORE200S || this->model_ == ModelType::CORE300S || this->model_ == ModelType::CORE400S || this->model_ == ModelType::CORE600S)
            {
                message = build_core_command(this, commandType);
            }
            else if (this->model_ == ModelType::VITAL100S || this->model_ == ModelType::VITAL200S || this->model_ == ModelType::EVERESTAIR)
            {
                message = build_vital_command(this, commandType);
            }
            else if (this->model_ == ModelType::SPROUT)
            {
                // Try Sprout-specific commands first, fall back to Vital for shared commands
                message = build_sprout_command(this, commandType);
                if (message.empty())
                    message = build_vital_command(this, commandType);
            }

            if (message.size() > 0)
            {
                ESP_LOGI(TAG, ">>> Sending command %s", command_type_to_string(commandType));
                ESP_LOGD(TAG, ">>> TX: %s", format_hex_pretty(message).c_str());
                this->write_array(message.data(), message.size());
                this->flush();
                // update the message counter
                if (messageUpCounter == 255)
                    messageUpCounter = 16;
                else
                    messageUpCounter++;
            }
        }
        /**
         * @brief Sends an acknowledgement message for the received ptype0/1, based on model type
         */

        // Superior 6000S: the panel's own filter-reset button expects a different
        // ack frame from the generic one.
        void Levoit::ackFilterReset(uint8_t ptype0, uint8_t ptype1)
        {
            uint8_t pv = 0x02; // Vital/Superior protocol version
            std::vector<uint8_t> message = {0xA5, 0x52, 0xFF, 0x04, 0x00, 0x00, pv, ptype0, ptype1, 0x16};
            levoit_finalize_message(message, messageUpCounter);

            if (message.size() > 0)
            {
                ESP_LOGI(TAG, ">>> Sending filter reset ack for: 0x%02X 0x%02X", ptype0, ptype1);
                this->write_array(message.data(), message.size());
                this->flush();
                if (messageUpCounter == 255)
                    messageUpCounter = 16;
                else
                    messageUpCounter++;
            }
        }

        // --- Superior 6000S ESP-managed timer ---
        void Levoit::start_esp_timer(uint32_t duration_secs)
        {
            esp_timer_active_ = true;
            esp_timer_start_millis_ = millis();
            esp_timer_duration_secs_ = duration_secs;
            esp_timer_last_update_ = esp_timer_start_millis_;
            esp_timer_zero_count_ = 0;
            ESP_LOGI(TAG, "ESP timer started: %u seconds", duration_secs);
        }

        void Levoit::stop_esp_timer()
        {
            esp_timer_active_ = false;
            esp_timer_zero_count_ = 0;
            ESP_LOGI(TAG, "ESP timer stopped");
        }

        // feature_id 0x19, payload 01 04 <LE32 seconds remaining>
        void Levoit::send_timer_update(uint32_t remaining_secs)
        {
            std::vector<uint8_t> msg_type = {0x02, 0x19, 0x50};
            std::vector<uint8_t> payload = {0x01, 0x04,
                                            (uint8_t)(remaining_secs & 0xFF),
                                            (uint8_t)((remaining_secs >> 8) & 0xFF),
                                            (uint8_t)((remaining_secs >> 16) & 0xFF),
                                            (uint8_t)((remaining_secs >> 24) & 0xFF)};

            auto message = build_levoit_message(msg_type, payload, messageUpCounter);
            if (message.size() > 0)
            {
                ESP_LOGD(TAG, ">>> TX timer update: %u sec remaining", remaining_secs);
                this->write_array(message.data(), message.size());
                this->flush();
                if (messageUpCounter == 255)
                    messageUpCounter = 16;
                else
                    messageUpCounter++;
            }
        }

        void Levoit::ackMessage(uint8_t ptype0, uint8_t ptype1)
        {

            uint8_t pv = 0x01;
            if (this->model_ == ModelType::VITAL100S || this->model_ == ModelType::VITAL200S || this->model_ == ModelType::SPROUT || this->model_ == ModelType::EVERESTAIR)
            {
                pv = 0x02;
                ESP_LOGI("TAG", ">>> Sending VITAL ack for: 0x%02X 0x%02X", ptype0, ptype1);
            }

            // Core600S state push (ptype 0x40/0x41) uses 0x52 with trailing 0x01, not a standard ACK
            if (this->model_ == ModelType::CORE600S && ptype0 == 0x40 && ptype1 == 0x41)
            {
                std::vector<uint8_t> message = {0xA5, 0x52, 0xFF, 0x04, 0xFF, 0x00, 0x01, ptype0, ptype1, 0x01};
                levoit_finalize_message(message, messageUpCounter);
                if (message.size() > 0)
                {
                    ESP_LOGI(TAG, ">>> Sending 0x52 recv for: 0x%02X 0x%02X", ptype0, ptype1);
                    this->write_array(message.data(), message.size());
                    this->flush();
                    if (messageUpCounter == 255) messageUpCounter = 16;
                    else messageUpCounter++;
                }
                return;
            }

            std::vector<uint8_t> message = {0xA5, 0x12, 0xFF, 0x04, 0xFF, 0x00, pv, ptype0, ptype1, 0x00};

            levoit_finalize_message(message, messageUpCounter);

            if (message.size() > 0)
            {
                ESP_LOGI(TAG, ">>> Sending ack  for: 0x%02X 0x%02X", ptype0, ptype1);

                this->write_array(message.data(), message.size());
                this->flush();
                // update the message counter
                if (messageUpCounter == 255)
                    messageUpCounter = 16;
                else
                    messageUpCounter++;
            }
        }

    } // namespace levoit
} // namespace esphome
