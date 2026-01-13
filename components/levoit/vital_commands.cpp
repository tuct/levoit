#include "vital_commands.h"
#include "levoit.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG_VITAL_CMD = "levoit.vital_cmd";

    extern std::vector<uint8_t> build_levoit_message(const std::vector<uint8_t> &msg_type, const std::vector<uint8_t> &payload);

    std::vector<uint8_t> build_vital_command(Levoit *self, CommandType cmd)
    {
      std::vector<uint8_t> message;

      switch (cmd)
      {
      case CommandType::setDeviceON:
        message = build_levoit_message({0x02, 0x00, 0x50}, {0x01, 0x01, 0x01});
        break;

      case CommandType::setDeviceOFF:
        message = build_levoit_message({0x02, 0x00, 0x50}, {0x01, 0x01, 0x00});
        break;

      case CommandType::setDeviceFanLvl1:
        message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x01});
        break;

      case CommandType::setDeviceFanLvl2:
        message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x02});
        break;

      case CommandType::setDeviceFanLvl3:
        message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x03});
        break;

      case CommandType::setDeviceFanLvl4:
        message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x04});
        break;

      case CommandType::setFanModeAuto:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x02});
        break;

      case CommandType::setFanModeSleep:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x01});
        break;

      case CommandType::setFanModeManual:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x00});
        break;

      case CommandType::setFanModePet:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x05});
        break;

      case CommandType::setLightDetectOn:
        message = build_levoit_message({0x02, 0x11, 0x55}, {0x01, 0x01, 0x01});
        break;

      case CommandType::setLightDetectOff:
        message = build_levoit_message({0x02, 0x11, 0x55}, {0x01, 0x01, 0x00});
        break;

      case CommandType::setDisplayOn:
        message = build_levoit_message({0x02, 0x04, 0x55}, {0x01, 0x01, 0x64});
        break;

      case CommandType::setDisplayOff:
        message = build_levoit_message({0x02, 0x04, 0x55}, {0x01, 0x01, 0x00});
        break;

      case CommandType::setDisplayLockOn:
        message = build_levoit_message({0x02, 0x40, 0x51}, {0x01, 0x01, 0x01});
        break;

      case CommandType::setDisplayLockOff:
        message = build_levoit_message({0x02, 0x40, 0x51}, {0x01, 0x01, 0x00});
        break;

      case CommandType::setAutoModeDefault:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x02, 0x01, 0x00, 0x03, 0x02, 0x00, 0x00});
        break;

      case CommandType::setAutoModeQuiet:
        message = build_levoit_message({0x02, 0x02, 0x55}, {0x02, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00});
        break;

      case CommandType::setAutoModeEfficient:
      {
        auto *num = self->get_number(NumberType::EFFICIENCY_ROOM_SIZE);
        if (num != nullptr)
        {
          uint32_t room_size = static_cast<uint32_t>(num->state);
          uint8_t size_low = room_size & 0xFF;
          uint8_t size_high = (room_size >> 8) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setAutoModeEfficient: room_size=%u size_low=0x%02X size_high=0x%02X",
                   (unsigned)room_size, size_low, size_high);

          message = build_levoit_message({0x02, 0x02, 0x55},
                                         {0x02, 0x01, 0x02, 0x03, 0x02, size_low, size_high});
        }
        else
        {
          message = build_levoit_message({0x02, 0x02, 0x55}, {0x02, 0x01, 0x02, 0x03, 0x02, 0xAD, 0x01});
        }
        break;
      }

      case CommandType::setTimerMinutes:
      {
        auto *num = self->get_number(NumberType::TIMER);
        if (num != nullptr)
        {
          uint32_t secs = static_cast<uint32_t>(num->state) * 60;
          uint8_t b0 = (secs >> 0) & 0xFF;
          uint8_t b1 = (secs >> 8) & 0xFF;
          uint8_t b2 = (secs >> 16) & 0xFF;
          uint8_t b3 = (secs >> 24) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",
                   (unsigned)secs, b0, b1, b2, b3);

          message = build_levoit_message({0x02, 0x19, 0x50}, {0x01, 0x04, b0, b1, b2, b3});

          if (secs > 0)
            self->start_timer();
          else
            self->stop_timer();
        }
        break;
      }

      case CommandType::requestTimerStatus:
        message = build_levoit_message({0x02, 0x1A, 0x50}, {});
        break;

      default:
        ESP_LOGW(TAG_VITAL_CMD, "Command not implemented: %s", command_type_to_string(cmd));
        break;
      }

      return message;
    }

  } // namespace levoit
} // namespace esphome
