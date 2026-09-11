#include "superior_status.h"
#include "esphome/core/log.h"
#include "tlv.h"
#include "types.h"
#include "fan/levoit_fan.h"
#include "sensor/levoit_sensor.h"
#include "decoder_helpers.h"
#include <vector>
#include <string>

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG_SUP = "levoit.superior";

    void decode_superior_status(Levoit *self,
                                ModelType model,
                                const uint8_t *payload,
                                size_t payload_len)
    {
      if (self == nullptr || !payload || payload_len == 0)
        return;

      std::vector<LevoitTLV> tlvs;
      bool ok = extract_tlvs_from_payload(model, payload, payload_len, tlvs, 0);
      if (!ok)
      {
        ESP_LOGW(TAG_SUP, "TLV extraction failed (payload_len=%u)", (unsigned)payload_len);
        return;
      }

      ESP_LOGV(TAG_SUP, "decode_superior_status: tlvs=%u", (unsigned)tlvs.size());

      bool have_power = false;
      bool have_mode = false;
      bool have_speed = false;
      bool have_dry_active = false;

      bool power = false;
      bool dry_active = false;
      uint32_t mode = 0;
      uint32_t speed = 0;

      for (const auto &t : tlvs)
      {
        switch (t.tag)
        {
        case 0x00:
          ESP_LOGV(TAG_SUP, "Power=%u", (unsigned)t.value_u32);
          have_power = true;
          power = (t.value_u32 == 1);
          break;

        case 0x01:
        {
          // MCU Version: 3 bytes (patch, minor, major)
          if (t.value_len >= 3 && t.value_ptr != nullptr)
          {
            uint8_t patch = t.value_ptr[0];
            uint8_t minor = t.value_ptr[1];
            uint8_t major = t.value_ptr[2];
            ESP_LOGV(TAG_SUP, "MCU_Version=%u.%u.%u (major.minor.patch)", major, minor, patch);
            self->publish_text_sensor(TextSensorType::MCU_VERSION,
                                      std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));
          }
          break;
        }

        case 0x03:
          ESP_LOGV(TAG_SUP, "CoverRemoved=%u", (unsigned)t.value_u32);
          self->publish_binary_sensor(BinarySensorType::COVER_OPEN, t.value_u32 == 1);
          break;

        case 0x04:
          ESP_LOGV(TAG_SUP, "WaterTankEmpty=%u", (unsigned)t.value_u32);
          self->publish_binary_sensor(BinarySensorType::WATER_TANK_EMPTY, t.value_u32 == 1);
          break;

        case 0x05:
        {
          // Display status block: 2 bytes - [0]=display setting, [1]=display current
          if (t.value_len >= 2 && t.value_ptr != nullptr)
          {
            uint8_t display_setting = t.value_ptr[0];
            uint8_t display_current = t.value_ptr[1];
            ESP_LOGV(TAG_SUP, "DisplaySetting=%u DisplayCurrent=%u", display_setting, display_current);
            self->publish_switch(SwitchType::DISPLAY, display_setting == 1);
          }
          else if (t.value_len == 1)
          {
            self->publish_switch(SwitchType::DISPLAY, t.value_u32 == 1);
          }
          break;
        }

        case 0x07:
          ESP_LOGV(TAG_SUP, "Humidifying=%u", (unsigned)t.value_u32);
          self->publish_binary_sensor(BinarySensorType::HUMIDIFYING, t.value_u32 == 1);
          break;

        case 0x08:
          ESP_LOGV(TAG_SUP, "HumiditySetpoint=%u%%", (unsigned)t.value_u32);
          self->publish_number(NumberType::HUMIDITY_TARGET, t.value_u32);
          break;

        case 0x09:
          ESP_LOGV(TAG_SUP, "HumidityCurrent=%u%%", (unsigned)t.value_u32);
          self->publish_sensor(SensorType::HUMIDITY, t.value_u32);
          break;

        case 0x0A:
          ESP_LOGV(TAG_SUP, "TemperatureInt=%u°C", (unsigned)t.value_u32);
          // Prefer 0x1E (precise temperature) when available
          break;

        case 0x0B:
        {
          // Main mode: 01=Manual, 02=Sleep, 03=Humidity, 04=Auto
          ESP_LOGV(TAG_SUP, "MainMode=%u", (unsigned)t.value_u32);
          have_mode = true;
          mode = t.value_u32;
          break;
        }

        case 0x0C:
          ESP_LOGV(TAG_SUP, "CurrentFanSpeed=%u", (unsigned)t.value_u32);
          have_speed = true;
          speed = t.value_u32;
          break;

        case 0x13:
        {
          // Auto profile: 01=Home, 02=Away -> select index 0=Home, 1=Away
          ESP_LOGV(TAG_SUP, "AutoProfile=%u", (unsigned)t.value_u32);
          if (t.value_u32 >= 1 && t.value_u32 <= 2)
            self->publish_select(SelectType::AUTO_PROFILE, t.value_u32 - 1);
          break;
        }

        case 0x14:
        {
          // Dry mode composite block: 5 bytes
          // bytes 0-2: remaining dry time (24-bit LE, seconds)
          // byte 3: dry active flag (01=active, 02=inactive)
          // byte 4: dry level (01=low, 02=high)
          if (t.value_len >= 5 && t.value_ptr != nullptr)
          {
            uint32_t dry_remaining = t.value_ptr[0] | (t.value_ptr[1] << 8) | (t.value_ptr[2] << 16);
            uint8_t dry_active_flag = t.value_ptr[3];
            uint8_t dry_level = t.value_ptr[4];

            ESP_LOGV(TAG_SUP, "DryRemaining=%u sec, DryActive=%u, DryLevel=%u",
                     dry_remaining, dry_active_flag, dry_level);

            // Active flag: 01=active, 02=inactive (unusual)
            have_dry_active = true;
            dry_active = (dry_active_flag == 1);
            self->publish_binary_sensor(BinarySensorType::DRY_ACTIVE, dry_active);

            // Dry level: 01=Low(idx 0), 02=High(idx 1)
            if (dry_level >= 1 && dry_level <= 2)
            {
              self->publish_select(SelectType::DRY_LEVEL, dry_level - 1);
              // Update the stored dry level preference
              self->set_dry_level_preference(dry_level - 1);
            }
          }
          break;
        }

        case 0x15:
          ESP_LOGV(TAG_SUP, "ManualFanSpeed=%u", (unsigned)t.value_u32);
          break;

        case 0x17:
          ESP_LOGV(TAG_SUP, "FilterLife=%u%%", (unsigned)t.value_u32);
          self->publish_sensor(SensorType::FILTER_LIFE_MCU, t.value_u32);
          break;

        case 0x18:
          ESP_LOGV(TAG_SUP, "DisplayLock=%u", (unsigned)t.value_u32);
          self->publish_switch(SwitchType::CHILD_LOCK, t.value_u32 == 1);
          break;

        case 0x1E:
        {
          // Precise temperature: LE16, divide by 10 for °C
          if (t.value_len >= 2 && t.value_ptr != nullptr)
          {
            uint16_t temp_x10 = t.value_ptr[0] | (t.value_ptr[1] << 8);
            float temp_c = temp_x10 / 10.0f;
            ESP_LOGV(TAG_SUP, "TemperaturePrecise=%.1f°C", temp_c);
            // Publish as integer (x10) to sensor, or as float
            auto *se = self->get_sensor(SensorType::TEMPERATURE);
            if (se != nullptr)
              se->publish_state(temp_c);
          }
          break;
        }

        case 0x21:
        {
          // Humidity subtype: 01=Smart, 02=Fan -> select index 0=Smart, 1=Fan
          ESP_LOGV(TAG_SUP, "HumiditySubtype=%u", (unsigned)t.value_u32);
          if (t.value_u32 >= 1 && t.value_u32 <= 2)
            self->publish_select(SelectType::HUMIDITY_SUBTYPE, t.value_u32 - 1);
          break;
        }

        // Known but not yet actionable fields
        case 0x06:
        case 0x11:
        case 0x16:
        case 0x1F:
        case 0x20:
        case 0x27:
          ESP_LOGV(TAG_SUP, "ReservedField=0x%02X val=%u", t.tag, (unsigned)t.value_u32);
          break;

        default:
          ESP_LOGV(TAG_SUP, "UnknownTag=0x%02X len=%u val=%u (0x%08X)",
                   t.tag, t.value_len, (unsigned)t.value_u32, (unsigned)t.value_u32);
          break;
        }
      }

      // Apply fan state
      auto *fan = self->get_fan();
      if (fan != nullptr)
      {
        int pwr = have_power ? (power ? 1 : 0) : -1;
        int spd = have_speed ? (int)speed : -1;
        // Map MCU mode values (1-based) to internal MODE_MAP values (0-based)
        // MCU: 1=Manual, 2=Sleep, 3=Humidity, 4=Auto
        // Internal: 0=Manual, 1=Sleep, 2=Auto, 3=Humidity
        int mod = -1;
        if (have_mode)
        {
          switch (mode)
          {
          case 1: mod = 0; break; // Manual
          case 2: mod = 1; break; // Sleep
          case 3: mod = 3; break; // Humidity
          case 4: mod = 2; break; // Auto
          default:
            ESP_LOGW(TAG_SUP, "Unexpected MCU mode value: %u", (unsigned)mode);
            break;
          }
        }
        // When dry mode is active, override the fan preset to "Dry" (device mode 6)
        if (have_dry_active && dry_active)
          mod = 6;
        ESP_LOGV(TAG_SUP, "Applying to fan: power=%d speed=%d mode=%d", pwr, spd, mod);
        fan->apply_device_status(pwr, spd, mod);
      }
    }

    void decode_superior_timer(Levoit *self,
                              ModelType model,
                              const uint8_t *payload,
                              size_t payload_len)
    {
      if (self == nullptr || !payload || payload_len == 0)
        return;

      std::vector<LevoitTLV> tlvs;
      bool ok = extract_tlvs_from_payload(model, payload, payload_len, tlvs, 0);
      if (!ok)
      {
        ESP_LOGW(TAG_SUP, "Timer TLV extraction failed (payload_len=%u)", (unsigned)payload_len);
        return;
      }

      uint32_t initial_secs = 0;
      bool have_initial = false;

      for (const auto &t : tlvs)
      {
        switch (t.tag)
        {
        case 0x01:
        {
          initial_secs = t.value_u32;
          have_initial = true;
          float initial_hours = initial_secs / 3600.0f;
          ESP_LOGV(TAG_SUP, "TimerInitial=%u sec (%.2f h)", (unsigned)initial_secs, initial_hours);
          self->publish_number(NumberType::TIMER, initial_hours);
          self->publish_text_sensor(TextSensorType::TIMER_DURATION_INITIAL, format_duration_minutes(initial_secs / 60));
          break;
        }
        case 0x02:
        {
          uint32_t remaining_secs = t.value_u32;
          float remaining_hours = remaining_secs / 3600.0f;
          uint16_t remaining_min = (uint16_t)remaining_secs / 60;
          ESP_LOGV(TAG_SUP, "TimerRemaining=%u sec (%.2f h)", (unsigned)remaining_secs, remaining_hours);
          self->publish_sensor(SensorType::TIMER_CURRENT, remaining_hours);
          self->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT, format_duration_minutes(remaining_min));

          if (remaining_secs > 0)
          {
            // Use initial duration if available, otherwise use remaining
            uint32_t duration = have_initial ? initial_secs : remaining_secs;
            self->start_esp_timer(duration);
          }
          else
          {
            self->stop_esp_timer();
            self->publish_number(NumberType::TIMER, 0.0f);
          }
          break;
        }
        default:
          break;
        }
      }
    }

  } // namespace levoit
} // namespace esphome
