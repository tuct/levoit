#include "levoit.h"
#include "esphome/components/network/util.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include "fan_modes.h"
#ifdef USE_LEVOIT_LIGHT
#include "light/levoit_light.h"
#endif
#ifdef USE_LEVOIT_SWITCH
#include "switch/levoit_switch.h"
#endif
#ifdef USE_LEVOIT_FAN
#include "fan/levoit_fan.h"
#endif
#ifdef USE_LEVOIT_NUMBER
#include "number/levoit_number.h"
#endif
#ifdef USE_LEVOIT_SENSOR
#include "sensor/levoit_sensor.h"
#endif
#ifdef USE_LEVOIT_SELECT
#include "select/levoit_select.h"
#endif
#ifdef USE_LEVOIT_TEXT_SENSOR
#include "text_sensor/levoit_text_sensor.h"
#endif

namespace esphome
{
    namespace levoit_component
    {

        static const char *const TAG = "levoit";

        // ===== Component =====

        void Levoit::setup()
        {
#ifdef USE_LEVOIT_TIME
            // Eleva 300X: hand the MCU the local time whenever the clock syncs.
            if (this->time_ != nullptr)
                this->time_->add_on_time_sync_callback([this]() { this->device_.on_time_synced(); });
#endif
            this->device_.begin();
        }

        void Levoit::loop()
        {
            // Hand the core what arrived. It frames, checks and decodes as it
            // goes, so a burst larger than its buffer is fine: it parses and
            // keeps taking bytes. A failed read ends this pass; the rest comes
            // next time around.
            uint8_t chunk[64];
            bool read_failed = false;
            while (!read_failed && available())
            {
                size_t n = 0;
                while (n < sizeof(chunk) && available())
                {
                    if (!read_byte(&chunk[n]))
                    {
                        read_failed = true;
                        break;
                    }
                    n++;
                }
                this->device_.receive(chunk, n, ::esphome::millis());
            }
            this->device_.tick(::esphome::millis());
            delay(1);
        }

        void Levoit::dump_config() {}

        // ===== entity registration (codegen) =====

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
            text_sensor_[tsl_idx_(type)] = tsl;
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

        // ===== LevoitHost: the ESPHome mapping of a changed slot =====
        // The entity dedup stays, so what reaches Home Assistant is what
        // reached it before the state moved into the core.

        void Levoit::switch_changed(SwitchType type, bool state)
        {
#ifdef USE_LEVOIT_SWITCH
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
#else
            (void) type;
            (void) state;
#endif
        }

        void Levoit::sensor_changed(SensorType type, float value)
        {
#ifdef USE_LEVOIT_SENSOR
            auto *se = sensors_[st_idx_(type)];
            if (!se)
                return;
            if (se->has_state() && se->state == value)
                return;
            se->publish_state(value);
#else
            (void) type;
            (void) value;
#endif
        }

        void Levoit::number_changed(NumberType type, float value)
        {
#ifdef USE_LEVOIT_NUMBER
            auto *nm = numbers_[nt_idx_(type)];
            if (!nm)
                return;
            value = nm->normalize_room_size(value);
            if (nm->has_state() && nm->state == value)
                return;
            nm->publish_state(value);
#else
            (void) type;
            (void) value;
#endif
        }
        void Levoit::select_changed(SelectType type, uint32_t value)
        {
#ifdef USE_LEVOIT_SELECT
            auto *sl = selects_[sl_idx_(type)];
            if (!sl)
                return;
            const auto &options = sl->traits.get_options();
            if (type == SelectType::FAN_OPERATING_MODE_SELECT || type == SelectType::DAYTIME_FAN_MODE)
            {
                const char *opt = fan_operating_mode_to_option(value);
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
#else
            (void) type;
            (void) value;
#endif
        }
        void Levoit::text_changed(TextSensorType type, const std::string &value)
        {
#ifdef USE_LEVOIT_TEXT_SENSOR
            auto *tsl = text_sensor_[static_cast<uint8_t>(type)];
            if (!tsl)
                return;
            if (tsl->has_state() && tsl->state == value)
                return;
            tsl->publish_state(value);
#else
            (void) type;
            (void) value;
#endif
        }
        void Levoit::binary_changed(BinarySensorType type, bool state)
        {
            // Nothing to push: LevoitBinarySensor::loop reads the slot through
            // get_binary_sensor_state and publishes from there.
            (void) type;
            (void) state;
        }
        void Levoit::fan_changed(int power, int speed, int mode)
        {
#ifdef USE_LEVOIT_FAN
            if (this->fan_ != nullptr)
                this->fan_->apply_device_status(power, speed, mode);
#else
            (void) power;
            (void) speed;
            (void) mode;
#endif
        }
        void Levoit::sprout_light_changed(bool on, float brightness, float color_temp, bool breathing)
        {
#ifdef USE_LEVOIT_LIGHT
            if (sprout_light_ != nullptr)
                sprout_light_->apply_from_mcu(on, brightness, color_temp, breathing);
#else
            (void) on;
            (void) brightness;
            (void) color_temp;
            (void) breathing;
#endif
        }

        // ===== LevoitPlatform =====

        void Levoit::write(const uint8_t *data, size_t len)
        {
            this->write_array(data, len);
            this->flush();
        }

        uint32_t Levoit::millis()
        {
            return ::esphome::millis();
        }

        void Levoit::set_timeout(const char *name, uint32_t ms, std::function<void()> fn)
        {
            Component::set_timeout(name, ms, std::move(fn));
        }

        void Levoit::cancel_timeout(const char *name)
        {
            Component::cancel_timeout(name);
        }

        // The same preference ids the component always used (fnv1 of the key),
        // so counters saved by earlier versions are found.
        ESPPreferenceObject *Levoit::pref_for_(const char *key)
        {
            if (std::string(key) == LevoitDevice::PREF_USED_CADR)
            {
                pref_used_cadr_ = global_preferences->make_preference<uint32_t>(fnv1_hash(key));
                return &pref_used_cadr_;
            }
            if (std::string(key) == LevoitDevice::PREF_TOTAL_RUNTIME)
            {
                pref_total_runtime_ = global_preferences->make_preference<uint32_t>(fnv1_hash(key));
                return &pref_total_runtime_;
            }
            ESP_LOGW(TAG, "No preference slot for key '%s'", key);
            return nullptr;
        }

        bool Levoit::load_u32(const char *key, uint32_t &out)
        {
            auto *pref = this->pref_for_(key);
            return pref != nullptr && pref->load(&out);
        }

        void Levoit::save_u32(const char *key, uint32_t value)
        {
            auto *pref = this->pref_for_(key);
            if (pref != nullptr)
                pref->save(&value);
        }

        bool Levoit::has_clock()
        {
#ifdef USE_LEVOIT_TIME
            return this->time_ != nullptr;
#else
            return false;
#endif
        }

        // Local time (UTC + zone offset), not UTC: what the Eleva's MCU expects.
        bool Levoit::local_time(uint32_t &epoch_local)
        {
#ifdef USE_LEVOIT_TIME
            if (this->time_ == nullptr)
                return false;
            ESPTime now = this->time_->now();
            if (!now.is_valid())
                return false;
            epoch_local = static_cast<uint32_t>(now.timestamp + ESPTime::timezone_offset());
            return true;
#else
            (void) epoch_local;
            return false;
#endif
        }

        NetworkState Levoit::network_state()
        {
            if (network::is_connected())
                return NetworkState::CONNECTED;
            if (network::is_disabled())
                return NetworkState::DISABLED;
            return NetworkState::CONNECTING;
        }

    } // namespace levoit_component
} // namespace esphome
