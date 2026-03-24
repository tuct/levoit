#include "core_commands.h"
#include "levoit_message.h"
#include "levoit.h"
#include "number/levoit_number.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace levoit
    {
        static const char *const TAG_CORE_CMD = "levoit.core_cmd";

        std::vector<uint8_t> build_core_command(Levoit *self, CommandType cmd)
        {
            std::vector<uint8_t> msg_type;
            std::vector<uint8_t> payload;

            switch (cmd)
            {
            case CommandType::setDeviceON:
                msg_type = {0x01, 0x00, 0xA0};
                payload = {0x01};
                break;

            case CommandType::setDeviceOFF:
                msg_type = {0x01, 0x00, 0xA0};
                payload = {0x00};
                break;

            case CommandType::setDeviceFanLvl1:
                msg_type = {0x01, 0x60, 0xA2};
                payload = {0x01, 0x01, 0x01};
                break;

            case CommandType::setDeviceFanLvl2:
                msg_type = {0x01, 0x60, 0xA2};
                payload = {0x01, 0x01, 0x02};
                break;

            case CommandType::setDeviceFanLvl3:
                msg_type = {0x01, 0x60, 0xA2};
                payload = {0x01, 0x01, 0x03};
                break;

            case CommandType::setDeviceFanLvl4:
                msg_type = {0x01, 0x60, 0xA2};
                payload = {0x01, 0x01, 0x04};
                break;

            case CommandType::setFanModeAuto:
                msg_type = {0x01, 0xE0, 0xA5};
                payload = {0x02};
                break;

            case CommandType::setFanModeSleep:
                msg_type = {0x01, 0xE0, 0xA5};
                payload = {0x01};
                break;

            case CommandType::setFanModeManual:
                msg_type = {0x01, 0xE0, 0xA5};
                payload = {0x00};
                break;

            case CommandType::setDisplayOn:
                msg_type = {0x01, 0x05, 0xA1};
                payload = {0x64};
                break;

            case CommandType::setDisplayOff:
                msg_type = {0x01, 0x05, 0xA1};
                payload = {0x00};
                break;

            case CommandType::setDisplayLockOn:
                msg_type = {0x01, 0x00, 0xD1};
                payload = {0x01};
                break;

            case CommandType::setDisplayLockOff:
                msg_type = {0x01, 0x00, 0xD1};
                payload = {0x00};
                break;

            case CommandType::setAutoModeDefault:
                msg_type = {0x01, 0xE6, 0xA5};
                payload = {0x00, 0x00, 0x00};
                break;

            case CommandType::setAutoModeQuiet:
                msg_type = {0x01, 0xE6, 0xA5};
                payload = {0x01, 0x00, 0x00};
                break;
            case CommandType::setWifiLedBlinking:
                msg_type = {0x01, 0x29, 0xA1};
                payload = {0x02, 0xF4, 0x01, 0xF4, 0x01, 0x00};
                break;
            case CommandType::setWifiLedOff:
                msg_type = {0x01, 0x29, 0xA1};
                payload = {0x00, 0xF4, 0x01, 0xF4, 0x01, 0x00};
                break;
            case CommandType::setWifiLedOn:
                msg_type = {0x01, 0x29, 0xA1};
                payload = {0x01, 0x7D, 0x00, 0x7D, 0x00, 0x00};
                break;
            case CommandType::setFilterLedOff:
                msg_type = {0x01, 0xE2, 0xA5};
                payload = {0x00};
                break;
            case CommandType::setFilterLedOn:
                msg_type = {0x01, 0xE2, 0xA5};
                payload = {0x01};
                break;
            case CommandType::setAutoModeEco:
                msg_type = {0x01, 0xE6, 0xA5};
                payload = {0x03, 0x00, 0x00};
                break;

            case CommandType::setAutoModeEfficient:
            {
                msg_type = {0x01, 0xE6, 0xA5};
                auto *num = self->get_number(NumberType::EFFICIENCY_ROOM_SIZE);
                if (num != nullptr)
                {
                    uint32_t room_size = static_cast<uint32_t>(num->state);
                    uint8_t size_low = room_size & 0xFF;
                    uint8_t size_high = (room_size >> 8) & 0xFF;

                    ESP_LOGD(TAG_CORE_CMD, "setAutoModeEfficient: room_size=%u size_low=0x%02X size_high=0x%02X",
                             (unsigned)room_size, size_low, size_high);

                    payload = {0x02, size_low, size_high};
                }
                else
                {
                    payload = {0x02, 0xAD, 0x01};
                }
                break;
            }

            case CommandType::setTimerStop:
                msg_type = {0x01, 0x64, 0xA2};
                payload = {0x00, 0x00, 0x00, 0x00};
                self->stop_timer();
                break;

            case CommandType::setTimerMinutes:
            {
                msg_type = {0x01, 0x64, 0xA2};
                auto *num = self->get_number(NumberType::TIMER);
                if (num != nullptr)
                {
                    uint32_t secs = static_cast<uint32_t>(num->state) * 60;
                    uint8_t b0 = (secs >> 0) & 0xFF;
                    uint8_t b1 = (secs >> 8) & 0xFF;
                    uint8_t b2 = (secs >> 16) & 0xFF;
                    uint8_t b3 = (secs >> 24) & 0xFF;

                    ESP_LOGD(TAG_CORE_CMD, "setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",
                             (unsigned)secs, b0, b1, b2, b3);

                    payload = {b0, b1, b2, b3};

                    if (secs > 0)
                        self->start_timer();
                    else
                        self->stop_timer();
                }
                break;
            }

            case CommandType::requestTimerStatus:
                msg_type = {0x01, 0x65, 0xA2};
                payload = {};
                break;

            // Core600S light detect
            case CommandType::setLightDetectOn:
                msg_type = {0x01, 0xE9, 0xA5};
                payload = {0x01};
                break;

            case CommandType::setLightDetectOff:
                msg_type = {0x01, 0xE9, 0xA5};
                payload = {0x00};
                break;

            // Core200S nightlight
            case CommandType::setNightlightOff:
                msg_type = {0x01, 0x03, 0xA0};
                payload = {0x00, 0x00};
                break;
            case CommandType::setNightlightMid:
                msg_type = {0x01, 0x03, 0xA0};
                payload = {0x00, 0x32};
                break;
            case CommandType::setNightlightFull:
                msg_type = {0x01, 0x03, 0xA0};
                payload = {0x00, 0x64};
                break;

            default:
                ESP_LOGW(TAG_CORE_CMD, "Command not implemented: %s", command_type_to_string(cmd));
                return {};
            }

            return build_levoit_message(msg_type, payload, messageUpCounter);
        }

    } // namespace levoit
} // namespace esphome
