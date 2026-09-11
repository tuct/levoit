#include "superior_commands.h"
#include "levoit_message.h"
#include "levoit.h"
#include "number/levoit_number.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG_SUP_CMD = "levoit.superior_cmd";

    std::vector<uint8_t> build_superior_command(Levoit *self, CommandType cmd)
    {
      std::vector<uint8_t> msg_type;
      std::vector<uint8_t> payload;

      switch (cmd)
      {
      // --- Power (feature_id = 0x00, protocol = 0x50) ---
      case CommandType::setDeviceON:
        msg_type = {0x02, 0x00, 0x50};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDeviceOFF:
        msg_type = {0x02, 0x00, 0x50};
        payload = {0x01, 0x01, 0x00};
        break;

      // --- Fan speed (feature_id = 0x33) ---
      case CommandType::setDeviceFanLvl1:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDeviceFanLvl2:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x02};
        break;

      case CommandType::setDeviceFanLvl3:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x03};
        break;

      case CommandType::setDeviceFanLvl4:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x04};
        break;

      case CommandType::setDeviceFanLvl5:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x05};
        break;

      case CommandType::setDeviceFanLvl6:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x06};
        break;

      case CommandType::setDeviceFanLvl7:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x07};
        break;

      case CommandType::setDeviceFanLvl8:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x08};
        break;

      case CommandType::setDeviceFanLvl9:
        msg_type = {0x02, 0x33, 0x55};
        payload = {0x01, 0x01, 0x09};
        break;

      // --- Main mode (feature_id = 0x32) ---
      case CommandType::setFanModeManual:
        msg_type = {0x02, 0x32, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setFanModeSleep:
        msg_type = {0x02, 0x32, 0x55};
        payload = {0x01, 0x01, 0x02};
        break;

      case CommandType::setFanModeHumidity:
        msg_type = {0x02, 0x32, 0x55};
        payload = {0x01, 0x01, 0x03};
        break;

      case CommandType::setFanModeAuto:
        msg_type = {0x02, 0x32, 0x55};
        payload = {0x01, 0x01, 0x04};
        break;

      // --- Auto profile (feature_id = 0x37, sub-id 0x01) ---
      case CommandType::setAutoProfileHome:
        msg_type = {0x02, 0x37, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setAutoProfileAway:
        msg_type = {0x02, 0x37, 0x55};
        payload = {0x01, 0x01, 0x02};
        break;

      // --- Humidity subtype (feature_id = 0x37, sub-id 0x02) ---
      case CommandType::setHumiditySubtypeSmart:
        msg_type = {0x02, 0x37, 0x55};
        payload = {0x02, 0x01, 0x01};
        break;

      case CommandType::setHumiditySubtypeFan:
        msg_type = {0x02, 0x37, 0x55};
        payload = {0x02, 0x01, 0x02};
        break;

      // --- Display (feature_id = 0x0F, protocol = 0x50) ---
      case CommandType::setDisplayOn:
        msg_type = {0x02, 0x0F, 0x50};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDisplayOff:
        msg_type = {0x02, 0x0F, 0x50};
        payload = {0x01, 0x01, 0x00};
        break;

      // --- Display lock (feature_id = 0x40, protocol = 0x51) ---
      case CommandType::setDisplayLockOn:
        msg_type = {0x02, 0x40, 0x51};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDisplayLockOff:
        msg_type = {0x02, 0x40, 0x51};
        payload = {0x01, 0x01, 0x00};
        break;

      // --- Humidity target (feature_id = 0x36) ---
      case CommandType::setHumidityTarget:
      {
        msg_type = {0x02, 0x36, 0x55};
        auto *num = self->get_number(NumberType::HUMIDITY_TARGET);
        if (num != nullptr)
        {
          uint8_t target = static_cast<uint8_t>(num->state);
          ESP_LOGD(TAG_SUP_CMD, "setHumidityTarget: %u%%", target);
          payload = {0x01, 0x01, target};
        }
        else
        {
          payload = {0x01, 0x01, 0x32}; // default 50%
        }
        break;
      }

      // --- Timer (feature_id = 0x19, protocol = 0x50) ---
      case CommandType::setTimerMinutes:
      {
        msg_type = {0x02, 0x19, 0x50};
        auto *num = self->get_number(NumberType::TIMER);
        if (num != nullptr)
        {
          uint32_t secs = static_cast<uint32_t>(num->state * 3600);
          uint8_t b0 = (secs >> 0) & 0xFF;
          uint8_t b1 = (secs >> 8) & 0xFF;
          uint8_t b2 = (secs >> 16) & 0xFF;
          uint8_t b3 = (secs >> 24) & 0xFF;

          ESP_LOGD(TAG_SUP_CMD, "setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",
                   (unsigned)secs, b0, b1, b2, b3);

          payload = {0x01, 0x04, b0, b1, b2, b3};
        }
        break;
      }

      // --- Reset filter (feature_id = 0x44) ---
      case CommandType::resetFilter:
        msg_type = {0x02, 0x44, 0x55};
        payload = {};
        break;

      // --- Dry level (feature_id = 0x46, sub-id 0x02) ---
      case CommandType::setDryLevelLow:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x02, 0x01, 0x01};
        break;

      case CommandType::setDryLevelHigh:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x02, 0x01, 0x02};
        break;

      // --- Auto-dry when powered off (feature_id = 0x46, sub-id 0x01) ---
      case CommandType::setAutoDryPowerOffOn:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setAutoDryPowerOffOff:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x01, 0x01, 0x00};
        break;

      // --- Auto-dry when out of water (feature_id = 0x46, sub-id 0x03) ---
      case CommandType::setAutoDryWaterEmptyOn:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x03, 0x01, 0x01};
        break;

      case CommandType::setAutoDryWaterEmptyOff:
        msg_type = {0x02, 0x46, 0x55};
        payload = {0x03, 0x01, 0x00};
        break;

      // --- WiFi LED (feature_id = 0x18, protocol = 0x50) ---
      case CommandType::setWifiLedBlinking:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x02, 0x02, 0x02, 0xF4, 0x01, 0x03, 0x02, 0xF4, 0x01, 0x04, 0x01, 0x00};
        break;

      case CommandType::setWifiLedOn:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x01, 0x02, 0x02, 0x7D, 0x00, 0x03, 0x02, 0x7D, 0x00, 0x04, 0x01, 0x00};
        break;

      case CommandType::setWifiLedOff:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x00, 0x02, 0x02, 0x4B, 0x00, 0x03, 0x02, 0x4B, 0x00, 0x04, 0x01, 0x00};
        break;

      default:
        ESP_LOGW(TAG_SUP_CMD, "Command not implemented: %s", command_type_to_string(cmd));
        return {};
      }

      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

  } // namespace levoit
} // namespace esphome
