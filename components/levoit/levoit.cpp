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
#include "freertos/task.h"
#include "decoder.h"
#include "tlv.h"
#include "vital_status.h"
#include "core_status.h"
#include "core_commands.h"
#include "vital_commands.h"
#include "sprout_commands.h"
#include "sprout_status.h"
#include "light/levoit_light.h"
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
#ifdef USE_SWITCH
            auto *sw = switches_[st_idx_(type)];
            if (!sw)
                return;
            if (sw->state == state)
                return;
            sw->publish_state(state);
#endif
        }

        void Levoit::publish_sensor(SensorType type, uint32_t value)
        {
#ifdef USE_SENSOR
            auto *se = sensors_[st_idx_(type)];
            if (!se)
                return;
            float fvalue = static_cast<float>(value);
            if (se->has_state() && se->state == fvalue)
                return;
            se->publish_state(value);
#endif
        }

        void Levoit::publish_number(NumberType type, uint32_t value)
        {
#ifdef USE_NUMBER
            auto *nm = numbers_[nt_idx_(type)];
            if (!nm)
                return;
            float fvalue = static_cast<float>(value);
            if (nm->has_state() && nm->state == fvalue)
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
            if (value >= options.size())
            {
                ESP_LOGW(TAG, "publish_select: invalid index %u for type %d (options=%u)",
                         value, (int)type, (unsigned)options.size());
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
        void Levoit::publish_sprout_light(bool on, float brightness, float color_temp, bool breathing)
        {
            if (sprout_light_ != nullptr)
                sprout_light_->apply_from_mcu(on, brightness, color_temp, breathing);
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

            // You’ll need to add command types for these if not present yet:
            case SwitchType::QUICK_CLEAN:
                // TODO: sendCommand(state ? setQuickCleanOn : setQuickCleanOff);
                break;

            case SwitchType::WHITE_NOISE:
                this->sendCommand(state ? setSproutWhiteNoiseOn : setSproutWhiteNoiseOff);
                break;

            case SwitchType::LED_RING:
                this->sendCommand(state ? setSproutLightNightlight : setSproutLedOff);
                break;

            default:
                break;
            }
        }

        void Levoit::on_number_command(NumberType type, uint32_t value)
        {
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == NumberType::EFFICIENCY_ROOM_SIZE) return;

            switch (type)
            {
            case NumberType::TIMER:
                if (this->model_ == ModelType::CORE200S) {
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
                break;

            case NumberType::LED_VALUE:
            case NumberType::LED_COLOR_TEMP:
                // Re-send the current light mode with updated value
                this->sendCommand(setSproutLightNightlight);
                break;

            case NumberType::LED_BRIGHTNESS_MIN:
            case NumberType::LED_SPEED:
                this->sendCommand(setSproutLightBreathing);
                break;

            case NumberType::WHITE_NOISE_VOLUME:
                this->sendCommand(setSproutWhiteNoiseOn);
                break;
            }
        }
        void Levoit::on_select_command(SelectType type, uint32_t value)
        {
            // Optional: restrict by model
            // if (model_ != ModelType::VITAL200S && type == SwitchType::QUICK_CLEAN) return;

            switch (type)
            {
            case SelectType::AUTO_MODE:
                switch (value)
                {
                case 0:
                    this->sendCommand(setAutoModeDefault);
                    break;
                case 1:
                    this->sendCommand(setAutoModeQuiet);
                    break;
                case 2:
                    this->sendCommand(setAutoModeEfficient);
                    break;
                case 3:
                    this->sendCommand(setAutoModeEco);
                    break;
                default:
                    break;
                }
                break;

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
                case 5:
                    this->sendCommand(setFanModePet);
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
                cadr = 415;
            if (model_ == ModelType::CORE200S)
                cadr = 167;
            if (model_ == ModelType::CORE600S)
                cadr = 641;
            if (model_ == ModelType::SPROUT)
                cadr = 230; // TODO: verify actual CADR from spec sheet
            
            // Initialize preferences for tracking used_cadr and total_runtime
            pref_used_cadr_ = global_preferences->make_preference<uint32_t>(fnv1_hash("levoit_used_cadr"));
            pref_total_runtime_ = global_preferences->make_preference<uint32_t>(fnv1_hash("levoit_runtime"));
            
            // Restore saved values or initialize to 0
            if (pref_used_cadr_.load(&used_cadr_)) {
                ESP_LOGI(TAG, "Restored used_cadr: %u m³", used_cadr_);
            } else {
                used_cadr_ = 0;
                ESP_LOGI(TAG, "Initialized used_cadr to 0");
            }
            if (pref_total_runtime_.load(&total_runtime_)) {
                ESP_LOGI(TAG, "Restored total_runtime: %u hours", total_runtime_);
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
                             cadr_per_min, speed, used_cadr_, total_runtime_);
                    
                }
                
                // Calculate and publish filter life left (once per minute here)
                float filter_left = this->calculate_filter_life_left_percent();
#ifdef USE_SENSOR
                auto *se = this->sensors_[st_idx_(SensorType::FILTER_LIFE_LEFT)];
                if (se != nullptr)
                    se->publish_state(filter_left);
#endif
                
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
            // Determine max speed based on model (Core300S has 3 speeds)
            uint32_t max_speed = (this->model_ == ModelType::CORE300S) ? 3u : 4u;
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
            }

            if (this->timer_active_ && now - last_check >= 10000)
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
            else if (this->model_ == ModelType::VITAL100S || this->model_ == ModelType::VITAL200S)
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
        void Levoit::ackMessage(uint8_t ptype0, uint8_t ptype1)
        {

            uint8_t pv = 0x01;
            if (this->model_ == ModelType::VITAL100S || this->model_ == ModelType::VITAL200S || this->model_ == ModelType::SPROUT)
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
