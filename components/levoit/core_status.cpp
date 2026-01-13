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
      if (self == nullptr || !payload || payload_len < 8)
        return;

      // Extract two uint32 values (little-endian)
      uint32_t remaining_sec = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
      uint32_t initial_sec = payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24);

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
      }
    }
    void decode_core_status(Levoit *self,
                            ModelType model,
                            const uint8_t *payload,
                            size_t payload_len)
    {
      (void)self;
      if (self == nullptr || !payload || payload_len == 0)
        return;

      // MCU version
      uint8_t patch = payload[0];
      uint8_t minor = payload[1];
      uint8_t major = payload[2];

      bool have_power = false;
      bool have_mode = false;
      bool have_speed = false;

      bool power = payload[3] != 0;
      uint32_t fan_mode = payload[4];
      uint8_t aqi = payload[10];
      float pm25 = static_cast<float>((payload[12] << 8) | payload[11]); // 2 bytes: little-endian (low byte at 11, high byte at 12)
      bool child_lock = payload[13] != 0;
      uint8_t fan_auto_mode = payload[14];
      uint16_t efficency_area = (payload[15] << 8) | payload[14]; // 2 bytes: little-endian (low byte at 14, high byte at 15)

      // display and fan speed differ between CORE300S and CORE400S or maybe based on firmware version?!?
      bool display_on = false;
      uint8_t fan_speed = 0;
      if (model == ModelType::CORE300S)
      {
        display_on = payload[7] != 0;
        fan_speed = payload[8];
      }
      else if (model == ModelType::CORE400S)
      {
        fan_speed = payload[5];
        display_on = payload[6] != 0;
      }

      // publish state to ESPHome entities
      self->publish_text_sensor(TextSensorType::MCU_VERSION, std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch));
      self->publish_switch(SwitchType::CHILD_LOCK, child_lock);
      self->publish_switch(SwitchType::DISPLAY, display_on);
      self->publish_sensor(SensorType::AQI, (unsigned)aqi);
      self->publish_sensor(SensorType::PM25, pm25);
      self->publish_select(SelectType::AUTO_MODE, fan_auto_mode);
      self->publish_number(NumberType::EFFICIENCY_ROOM_SIZE, (unsigned)efficency_area);
      auto *fan = self->get_fan();
      if (fan != nullptr)
      {
        ESP_LOGV(TAG_CORE, "Applying to fan: power=%d speed=%d mode=%d", power, fan_speed, fan_mode);
        fan->apply_device_status(power, fan_speed, fan_mode);
      }
    }

  } // namespace levoit
} // namespace esphome
