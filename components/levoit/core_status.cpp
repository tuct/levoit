#include "core_status.h"
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

    static const char *const TAG_CORE = "levoit.core";

    void decode_core_timer(Levoit *self,
                           ModelType model,
                           const uint8_t *payload,
                           size_t payload_len)
    {
      (void)model;
      if (self == nullptr || !payload || payload_len < 4)
        return;

      // Extract two uint32 values (little-endian).
      // Core200S echoes only remaining_sec (4 bytes); Core300S sends both (8 bytes).
      uint32_t remaining_sec = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
      uint32_t initial_sec = (payload_len >= 8)
          ? (payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24))
          : remaining_sec;

      uint16_t remaining_min = remaining_sec / 60;
      uint16_t initial_min = initial_sec / 60;

      ESP_LOGV(TAG_CORE, "Timer: remaining=%u sec (%u min), initial=%u sec (%u min)", 
               remaining_sec, remaining_min, initial_sec, initial_min);

      self->publish_number(NumberType::TIMER, initial_min);
      self->publish_text_sensor(TextSensorType::TIMER_DURATION_INITIAL, format_duration_minutes(initial_min));
      self->publish_sensor(SensorType::TIMER_CURRENT, remaining_min);
      self->publish_text_sensor(TextSensorType::TIMER_DURATION_CURRENT, format_duration_minutes(remaining_min));

      if (remaining_min > 0)
      {
        self->start_timer();
      }
      else
      {
        if (self->is_timer_active())
        {
          self->stop_timer();
          self->publish_number(NumberType::TIMER, 0);
        }
        self->set_timer_stop_pending(false);  // MCU confirmed timer off
      }
    }
    void decode_core_status(Levoit *self,
                            ModelType model,
                            const uint8_t *payload,
                            size_t payload_len)
    {
      if (self == nullptr || !payload || payload_len == 0)
        return;

      uint8_t patch = payload[0];
      uint8_t minor = payload[1];
      uint8_t major = payload[2];
      bool power = payload[3] != 0;
      uint32_t fan_mode = payload[4];

      self->publish_text_sensor(TextSensorType::MCU_VERSION,
          std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));

      // --- Core200S: different byte layout, no PM2.5/AQI/auto mode ---
      if (model == ModelType::CORE200S)
      {
        if (payload_len < 12)
          return;
        bool display_on = payload[6] != 0;
        uint8_t fan_speed = payload[5];
        bool child_lock = payload[10] != 0;  // Display Lock = child lock
        uint8_t nightlight_raw = payload[11]; // 0x00=off, 0x32=mid, 0x64=full
        uint8_t nightlight_idx = (nightlight_raw == 0x32) ? 1 : (nightlight_raw == 0x64) ? 2 : 0;

        self->publish_switch(SwitchType::DISPLAY, display_on);
        self->publish_switch(SwitchType::CHILD_LOCK, child_lock);
        self->publish_select(SelectType::NIGHTLIGHT, nightlight_idx);

        auto *fan = self->get_fan();
        if (fan != nullptr)
        {
          ESP_LOGV(TAG_CORE, "Core200S fan: power=%d speed=%d mode=%d", power, fan_speed, (int)fan_mode);
          fan->apply_device_status(power, fan_speed, fan_mode);
        }
        return;
      }

      // --- Core600S ---
      if (model == ModelType::CORE600S)
      {
        if (payload_len < 22)
          return;

        uint8_t fan_speed   = payload[5];
        bool display_on     = payload[8] != 0;
        uint8_t aqi         = payload[11];
        uint16_t pm25_raw   = (uint16_t)((payload[13] << 8) | payload[12]);
        float pm25          = static_cast<float>(pm25_raw);
        bool child_lock     = payload[14] != 0;
        uint8_t fan_auto_mode = payload[15];
        uint16_t efficency_area = (uint16_t)((payload[17] << 8) | payload[16]);
        float efficency_area_m2 = static_cast<float>(efficency_area) / (10.764f * 3.15f);
        bool light_detect   = payload[21] != 0;

        self->publish_text_sensor(TextSensorType::ERROR_MESSAGE, "Ok");
        self->publish_switch(SwitchType::CHILD_LOCK, child_lock);
        self->publish_switch(SwitchType::DISPLAY, display_on);
        self->publish_switch(SwitchType::LIGHT_DETECT, light_detect);
        self->publish_sensor(SensorType::AQI, (unsigned)aqi);
        if (pm25_raw <= 1000)
          self->publish_sensor(SensorType::PM25, pm25);
        else
          ESP_LOGW(TAG_CORE, "PM2.5 raw value %u out of range, skipping publish", pm25_raw);
        self->publish_select(SelectType::AUTO_MODE, fan_auto_mode);
        self->publish_number(NumberType::EFFICIENCY_ROOM_SIZE, efficency_area_m2);

        auto *fan = self->get_fan();
        if (fan != nullptr)
        {
          ESP_LOGV(TAG_CORE, "Core600S fan: power=%d speed=%d mode=%d", power, fan_speed, (int)fan_mode);
          fan->apply_device_status(power, fan_speed, fan_mode);
        }
        return;
      }

      // --- Core300S / Core400S ---
      if (payload_len < 18)
        return;

      uint8_t aqi = payload[10];
      uint16_t pm25_raw = (payload[12] << 8) | payload[11];
      float pm25 = static_cast<float>(pm25_raw);
      bool child_lock = payload[13] != 0;
      uint8_t fan_auto_mode = payload[14];
      uint16_t efficency_area = (payload[16] << 8) | payload[15];
      float efficency_area_m2 = static_cast<float>(efficency_area) / (10.764f * 3.15f);
      bool has_error = payload[17] != 0;

      bool display_on = false;
      uint8_t fan_speed = 0;
      if (model == ModelType::CORE300S)
      {
        display_on = payload[6] != 0;
        fan_speed = payload[5];
      }
      else if (model == ModelType::CORE400S)
      {
        fan_speed = payload[5];
        display_on = payload[7] != 0;
      }

      if (has_error)
        self->publish_text_sensor(TextSensorType::ERROR_MESSAGE, "Sensor error");
      else
        self->publish_text_sensor(TextSensorType::ERROR_MESSAGE, "Ok");

      self->publish_switch(SwitchType::CHILD_LOCK, child_lock);
      self->publish_switch(SwitchType::DISPLAY, display_on);
      self->publish_sensor(SensorType::AQI, (unsigned)aqi);
      if (pm25_raw <= 1000)
        self->publish_sensor(SensorType::PM25, pm25);
      else
        ESP_LOGW(TAG_CORE, "PM2.5 raw value %u out of range, skipping publish", pm25_raw);
      self->publish_select(SelectType::AUTO_MODE, fan_auto_mode);
      self->publish_number(NumberType::EFFICIENCY_ROOM_SIZE, efficency_area_m2);

      auto *fan = self->get_fan();
      if (fan != nullptr)
      {
        ESP_LOGV(TAG_CORE, "Applying to fan: power=%d speed=%d mode=%d", power, fan_speed, (int)fan_mode);
        fan->apply_device_status(power, fan_speed, fan_mode);
      }
    }

  } // namespace levoit
} // namespace esphome
