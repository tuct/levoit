#include "vital_status.h"
#include "esphome/core/log.h"
#include "tlv.h" // <- TLV extraction
#include "types.h"
#include "fan/levoit_fan.h"
#include "decoder_helpers.h"
#include <vector>
#include <string>

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG_VITAL = "levoit.vital"; 


    static inline uint8_t b_(const LevoitTLV &t, size_t idx)
    {
      return (t.value_ptr && idx < t.value_len) ? t.value_ptr[idx] : 0;
    }
    void decode_vital_timer(Levoit *self,
                            ModelType model,
                            const uint8_t *payload,
                            size_t payload_len)
    {
      (void)self;
      if (!payload || payload_len == 0)
        return;

      std::vector<LevoitTLV> tlvs;
      bool ok = extract_tlvs_from_payload(model, payload, payload_len, tlvs, 0);
      if (!ok)
      {
        ESP_LOGW(TAG_VITAL, "TLV extraction failed (payload_len=%u)", (unsigned)payload_len);
        return;
      }
      // Map Vital response to compoenent state
      for (const auto &t : tlvs)
      {
        switch (t.tag)
        {
        case 0x01:
        {
          uint16_t initial_min = (uint16_t)t.value_u32 / 60;
          ESP_LOGV(TAG_VITAL, "Initial=%u (0x%04X), min = %u", (unsigned)t.value_u32, (unsigned)t.value_u32, initial_min);
          if (self != nullptr){
            self->publish_number(NumberType::TIMER, initial_min);
            self->publish_text_sensor(TextSensorType::TIMER_DURATION_INITIAL, format_duration_minutes(initial_min));
          }
            

          break;
        }
        case 0x02:
        {
          uint16_t remaining_min = (uint16_t)t.value_u32 / 60;

          if (self != nullptr)
          {
            self->publish_sensor(SensorType::TIMER_CURRENT, remaining_min);
            self->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT, format_duration_minutes(remaining_min));
            if (remaining_min > 0)
            {
              self->start_timer();
            }
            else
            {
              if (self->is_timer_active()){
                self->stop_timer();
                self->publish_number(NumberType::TIMER, 0); // set to 0 when timer ends
              }
              self->set_timer_stop_pending(false);  // MCU confirmed timer off
            }
          }

          ESP_LOGV(TAG_VITAL, "remaining=%u (0x%04X), min = %u", (unsigned)t.value_u32, (unsigned)t.value_u32, remaining_min);
          break;
        }
        default:
          break;
        }
      }
    }
    void decode_vital_status(Levoit *self,
                             ModelType model,
                             const uint8_t *payload,
                             size_t payload_len)
    {
      (void)self;
      if (!payload || payload_len == 0)
        return;

      std::vector<LevoitTLV> tlvs;
      bool ok = extract_tlvs_from_payload(model, payload, payload_len, tlvs, 0);
      if (!ok)
      {
        ESP_LOGW(TAG_VITAL, "TLV extraction failed (payload_len=%u)", (unsigned)payload_len);
        return;
      }

      ESP_LOGV(TAG_VITAL, "decode_vital_status: tlvs=%u", (unsigned)tlvs.size());

      bool have_power = false;
      bool have_mode = false;
      bool have_speed = false;

      bool power = false;
      uint32_t mode = 0;
      uint32_t speed = 0; // choose FanLevel or FanSpeed, see notes below

      // Sprout LED ring state — collected across tag24/tag25/tag26, applied after loop
      bool sprout_nightlight_on = false;
      bool sprout_breathing_on = false;
      uint16_t sprout_nightlight_ct = 0;   // Kelvin
      uint8_t sprout_nightlight_bri = 0;   // 0–100 pct
      bool sprout_breathing_params_known = false;
      uint8_t sprout_breathing_speed = 0;
      uint8_t sprout_breathing_max = 0;    // 0–100 pct
      uint8_t sprout_breathing_min = 0;    // 0–100 pct
      uint16_t sprout_breathing_ct = 0;    // Kelvin

      // Map Vital response to compoenent state
      for (const auto &t : tlvs)
      {
        switch (t.tag)
        {
        case 0x00:
          ESP_LOGV(TAG_VITAL, "ID=%u (0x%02X)", (unsigned)t.value_u32, (unsigned)t.value_u32);
          // TODO
          break;

        case 0x01:
        {
          uint8_t patch = b_(t, 0);
          uint8_t minor = b_(t, 1);
          uint8_t major = b_(t, 2);
          ESP_LOGV(TAG_VITAL, "MCU_Version=%u.%u.%u (major.minor.patch)", major, minor, patch);
          if (self != nullptr)
            self->publish_text_sensor(TextSensorType::MCU_VERSION, std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));

          break;
        }
        case 0x02:
          ESP_LOGV(TAG_VITAL, "Power=%u", (unsigned)t.value_u32);
          have_power = true;
          power = (t.value_u32 == 1);
          break;
        case 0x03:
          ESP_LOGV(TAG_VITAL, "FanMode=%u", (unsigned)t.value_u32);
          have_mode = true;
          mode = t.value_u32;
          break;
        case 0x04:
          ESP_LOGV(TAG_VITAL, "FanLevel=%u", (unsigned)t.value_u32);
          have_speed = true;
          speed = t.value_u32;
          break;
        case 0x05:
          ESP_LOGV(TAG_VITAL, "FanSpeed=%u", (unsigned)t.value_u32);
          break;
        case 0x06:
        {
          ESP_LOGV(TAG_VITAL, "DisplayIlluminated=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_switch(SwitchType::DISPLAY, t.value_u32 == 1);
          break;
        }
        case 0x07:
        {
          ESP_LOGV(TAG_VITAL, "DisplayState=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        }
        case 0x08:
          ESP_LOGV(TAG_VITAL, "Unknown_08=%u", (unsigned)t.value_u32);
          break;
        case 0x09:
          ESP_LOGV(TAG_VITAL, "AirQualityLevel=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_sensor(SensorType::AQI, (unsigned)t.value_u32);
          break;
        case 0x0A:
          ESP_LOGV(TAG_VITAL, "AirQualityDetail=%u", (unsigned)t.value_u32);
          if( self != nullptr){
            if((unsigned)t.value_u32 == 0){
              self->publish_text_sensor(TextSensorType::ERROR_MESSAGE, "Sensor Error");
            }else{
              self->publish_text_sensor(TextSensorType::ERROR_MESSAGE, "Ok");
            }
          }
            
          break;
        case 0x0B:
          ESP_LOGV(TAG_VITAL, "PM2.5=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_sensor(SensorType::PM25, (unsigned)t.value_u32);
          break;
        case 0x0E:
        {
          ESP_LOGV(TAG_VITAL, "ChildLock=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_switch(SwitchType::CHILD_LOCK, t.value_u32 == 1);
          break;
        }
        case 0x0F:
          ESP_LOGV(TAG_VITAL, "AutoMode=%u", (unsigned)t.value_u32);
          // TODO!
          if (self != nullptr)
          {
            // map AutoMode to Select index, in this case both are identical
            uint8_t auto_mode = static_cast<uint8_t>(t.value_u32);
            uint8_t select_idx;
            if (auto_mode < 3)
            {
              select_idx = auto_mode;
            }
            else
            {
              ESP_LOGW(TAG_VITAL, "AUTO_MODE_MAP: index out of range: %u", auto_mode);
              select_idx = 0; // fallback to "Default"
            }

            /*
            static constexpr uint8_t AUTO_MODE_MAP[] = {0, 1, 2};


            if (auto_mode < sizeof(AUTO_MODE_MAP)) {
              ESP_LOGV(TAG_VITAL, "AUTO_MODE_MAP: mapping %u to %u", (unsigned)auto_mode, (unsigned)AUTO_MODE_MAP[auto_mode]);
              select_idx = AUTO_MODE_MAP[auto_mode];
            } else {
              ESP_LOGW(TAG_VITAL, "AUTO_MODE_MAP: index out of range: %u", auto_mode);
              select_idx = AUTO_MODE_MAP[0];  // fallback to "Default"
            }
            */
            self->publish_select(SelectType::AUTO_MODE, select_idx);
          }

          break;
        case 0x10:
        {
          // raw = sq_ft × 1.3, convert back to m²: m² = raw / (10.764 × 1.3)
          float efficency_area_m2 = static_cast<float>(t.value_u32) / (10.764f * 1.3f);
          ESP_LOGV(TAG_VITAL, "EfficientValue=%u (0x%04X) -> %.1f m²", (unsigned)t.value_u32, (unsigned)t.value_u32, efficency_area_m2);
          if (self != nullptr)
            self->publish_number(NumberType::EFFICIENCY_ROOM_SIZE, efficency_area_m2);
          break;
        }
        case 0x11:
          ESP_LOGV(TAG_VITAL, "EfficientCounter=%u (0x%04X)", (unsigned)t.value_u32, (unsigned)t.value_u32);
          if (self != nullptr){
            self->publish_text_sensor(TextSensorType::AUTO_MODE_ROOM_SIZE_HIGH_FAN, format_duration_seconds((unsigned)t.value_u32) );
            self->publish_sensor(SensorType::EFFICIENCY_COUNTER, (unsigned)t.value_u32);
          }
           
          break;
        case 0x12:
          ESP_LOGV(TAG_VITAL, "AutoModeProfile=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        case 0x13:
        {
          ESP_LOGV(TAG_VITAL, "LightDetect=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_switch(SwitchType::LIGHT_DETECT, t.value_u32 == 1);
          break;
        }
        case 0x16:
          ESP_LOGV(TAG_VITAL, "WifiLight=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        case 0x17:
          ESP_LOGV(TAG_VITAL, "Dark Dedected=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        case 0x18:
          ESP_LOGV(TAG_VITAL, "SleepModeType=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        case 0x19:
        {
          ESP_LOGV(TAG_VITAL, "QuickCleanEnabled=%u", (unsigned)t.value_u32);
          // TODO!
          break;
        }
        // TODO!
        case 0x1A:
          ESP_LOGV(TAG_VITAL, "QuickCleanMinutes=%u", (unsigned)t.value_u32);
          break;
        case 0x1B:
          ESP_LOGV(TAG_VITAL, "QuickCleanFanLevel=%u", (unsigned)t.value_u32);
          break;
        case 0x1C:
          ESP_LOGV(TAG_VITAL, "WhiteNoiseEnabled=%u", (unsigned)t.value_u32);
          break;
        case 0x1D:
          ESP_LOGV(TAG_VITAL, "WhiteNoiseMinutes=%u", (unsigned)t.value_u32);
          break;
        case 0x1E:
          ESP_LOGV(TAG_VITAL, "WhiteNoiseFanLevel=%u", (unsigned)t.value_u32);
          break;
        case 0x1F:
          ESP_LOGV(TAG_VITAL, "SleepFanModeOrLevel=%u", (unsigned)t.value_u32);
          break;
        case 0x20:
          ESP_LOGV(TAG_VITAL, "SleepModeMinutes=%u", (unsigned)t.value_u32);
          break;
        case 0x21:
          ESP_LOGV(TAG_VITAL, "DaytimeEnabled=%u", (unsigned)t.value_u32);
          break;
        case 0x22:
          ESP_LOGV(TAG_VITAL, "DaytimeFanMode=%u", (unsigned)t.value_u32);
          break;
        case 0x23:
          ESP_LOGV(TAG_VITAL, "DaytimeFanLevel=%u", (unsigned)t.value_u32);
          break;

        // Sprout-specific tags (also present in CMD=02 00 55, same TLV format)
        case 0x0C:
          ESP_LOGV(TAG_VITAL, "PM1.0_raw=%u", (unsigned)t.value_u32);
          if (self != nullptr && model == ModelType::SPROUT)
            self->publish_sensor(SensorType::PM1_0, (unsigned)t.value_u32);
          break;
        case 0x0D:
          ESP_LOGV(TAG_VITAL, "PM10_raw=%u", (unsigned)t.value_u32);
          if (self != nullptr && model == ModelType::SPROUT)
            self->publish_sensor(SensorType::PM10, (unsigned)t.value_u32);
          break;
        case 0x24:
          // Sprout: 4-byte nightlight state: {on, brightness_pct, ct_lo, ct_hi}
          //   byte0: 0x01=nightlight active, 0x00=off
          //   byte1: brightness 0–100 (uint8 percentage)
          //   bytes 2-3: color temperature Kelvin LE16 (e.g. 0x07D0=2000K, 0x0DAC=3500K)
          // Confirmed from captures: changes with CMD=02 0B 55 nightlight commands
          if (t.value_len == 4 && t.value_ptr != nullptr && model == ModelType::SPROUT)
          {
            sprout_nightlight_on  = (t.value_ptr[0] != 0);
            sprout_nightlight_bri = t.value_ptr[1];  // 0–100 pct
            sprout_nightlight_ct  = (uint16_t)t.value_ptr[2] | ((uint16_t)t.value_ptr[3] << 8);  // Kelvin
            ESP_LOGV(TAG_VITAL, "Sprout Nightlight: on=%u bri_pct=%u ct_k=%u",
                     sprout_nightlight_on, sprout_nightlight_bri, sprout_nightlight_ct);
          }
          break;
        case 0x25:
          // Sprout: 6-byte breathing state: {mode, speed_sec, ct_lo, ct_hi, min_pct, max_pct}
          //   mode=0x01 → breathing active; mode=0x00 → breathing inactive (off or nightlight)
          //   speed_sec: breathing cycle time in seconds (1–10)
          //   ct_lo/ct_hi: color temperature Kelvin LE16
          //   min_pct: minimum brightness 0–100
          //   max_pct: maximum brightness 0–100
          // LIGHT_MODE requires combining tag24+tag25+tag26 — applied after loop
          if (t.value_len == 6 && t.value_ptr != nullptr && model == ModelType::SPROUT)
          {
            sprout_breathing_on    = (t.value_ptr[0] == 0x01);
            sprout_breathing_speed = t.value_ptr[1];
            sprout_breathing_ct    = (uint16_t)t.value_ptr[2] | ((uint16_t)t.value_ptr[3] << 8);  // Kelvin
            sprout_breathing_min   = t.value_ptr[4];  // 0–100 pct
            sprout_breathing_max   = t.value_ptr[5];  // 0–100 pct
            sprout_breathing_params_known = true;
            ESP_LOGV(TAG_VITAL, "Sprout Breathing: on=%u speed=%u ct_k=%u min_pct=%u max_pct=%u",
                     sprout_breathing_on, sprout_breathing_speed,
                     sprout_breathing_ct, sprout_breathing_min, sprout_breathing_max);
          }
          break;
        case 0x26:
          // Sprout: breathing-active indicator (mirrors tag25 mode byte)
          // 0x01 = breathing currently running; 0x00 = nightlight or off
          ESP_LOGV(TAG_VITAL, "Sprout BreathingActive=%u", (unsigned)t.value_u32);
          // (state is derived from tag25 above; this tag is informational only)
          break;
        case 0x27:
          ESP_LOGV(TAG_VITAL, "FanRPM=%u", (unsigned)t.value_u32);
          if (self != nullptr && model == ModelType::SPROUT)
            self->publish_sensor(SensorType::FAN_RPM, (unsigned)t.value_u32);
          break;

        default:
          ESP_LOGV(TAG_VITAL, "UnknownTag=0x%02X len=%u val=%u (0x%08X)",
                   t.tag, t.value_len, (unsigned)t.value_u32, (unsigned)t.value_u32);
          break;
        }
      }
      // Sprout: apply LED ring state from combined tag24/tag25/tag26 readings
      if (self != nullptr && model == ModelType::SPROUT)
      {
        bool led_on = sprout_nightlight_on || sprout_breathing_on;
        self->publish_switch(SwitchType::LED_RING, led_on);

        float brightness = 0.0f;
        if (sprout_breathing_on && sprout_breathing_params_known)
        {
          brightness = sprout_breathing_max / 100.0f;
          self->publish_number(NumberType::LED_BRIGHTNESS_MIN, sprout_breathing_min);
          self->publish_number(NumberType::LED_SPEED, sprout_breathing_speed);
        }
        else if (sprout_nightlight_on)
        {
          brightness = sprout_nightlight_bri / 100.0f;
        }

        // Update light entity (also keeps LIGHT_MODE select in sync if still registered)
        self->publish_sprout_light(led_on, brightness, 0.0f, sprout_breathing_on);
        self->publish_select(SelectType::LIGHT_MODE,
                             sprout_breathing_on ? 2 : (sprout_nightlight_on ? 1 : 0));
      }

      // Apply to ESPHome fan once, with consistent values
      auto *fan = (self != nullptr) ? self->get_fan() : nullptr;
      if (fan != nullptr)
      {
        // Use sentinels: power_known, speed_known, mode_known
        // - power: pass 0/1 only if have_power, otherwise pass -1
        // - speed: pass >0 only if have_speed, otherwise -1
        // - mode : pass >=0 only if have_mode, otherwise -1

        int pwr = have_power ? (power ? 1 : 0) : -1;
        int spd = have_speed ? (int)speed : -1;
        int mod = have_mode ? (int)mode : -1;
        ESP_LOGV(TAG_VITAL, "Applying to fan: power=%d speed=%d mode=%d", pwr, spd, mod);
        fan->apply_device_status(pwr, spd, mod);
      }
    }

  } // namespace levoit
} // namespace esphome
