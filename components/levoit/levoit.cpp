#include "levoit.h"
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
#include "freertos/task.h"
#include "decoder.h"
#include "tlv.h"
#include "vital_status.h"
#include "switch/levoit_switch.h"
#include "fan/levoit_fan.h"
#include "number/levoit_number.h"
#include "sensor/levoit_sensor.h"
#include "select/levoit_select.h"
#include "text_sensor/levoit_text_sensor.h"

// could be that less values generat errors(but not tested troughoutly).
std::uint8_t messageUpCounter = 16;

namespace esphome
{
    namespace levoit
    {

        static const char *const TAG = "levoit";
        constexpr size_t LEVOIT_IDX_COUNTER = 2;
        constexpr size_t LEVOIT_IDX_LENGTH = 3;
        constexpr size_t LEVOIT_IDX_CHECKSUM = 5;

        static uint8_t levoit_checksum(const uint8_t *data, size_t len)
        {
            uint32_t sum = 0;
            for (size_t i = 0; i < len; i++)
            {
                if (i == 5)
                    continue; // skip checksum byte
                sum += data[i];
            }
            return (uint8_t)(0xFF - (sum & 0xFF));
        }
        // add counter and checksum
        static void levoit_finalize_message(
            std::vector<uint8_t> &message)
        {
            // Sanity check
            if (message.size() <= LEVOIT_IDX_CHECKSUM)
                return;

            // Set message counter
            message[LEVOIT_IDX_COUNTER] = messageUpCounter;

            // Compute checksum (skip checksum byte itself)
            uint32_t sum = 0;
            for (size_t i = 0; i < message.size(); i++)
            {
                if (i == LEVOIT_IDX_CHECKSUM)
                    continue;
                sum += message[i];
            }

            message[LEVOIT_IDX_CHECKSUM] = static_cast<uint8_t>(0xFF - (sum & 0xFF));
        }

        static std::vector<uint8_t> build_levoit_message(
            const std::vector<uint8_t> &msg_type,
            const std::vector<uint8_t> &payload)
        {
            std::vector<uint8_t> msg;

            // ---- header (checksum placeholder at [5]) ----
            msg.push_back(0xA5);                                     // 0
            msg.push_back(0x22);                                     // 1
            msg.push_back(messageUpCounter);                         // 2
            msg.push_back(0xFF);                                     // 3 length (filled later)
            msg.push_back(0x00);                                     // 4 fixed ´0x00´
            msg.push_back(0xFF);                                     // 5 checksum placeholder
            msg.insert(msg.end(), msg_type.begin(), msg_type.end()); // 6,7,8
            msg.push_back(0x00);                                     // 9
            // ---- payload ----
            if (!payload.empty())
                msg.insert(msg.end(), payload.begin(), payload.end());

            // ---- length = bytes AFTER checksum ----
            msg[LEVOIT_IDX_LENGTH] = static_cast<uint8_t>(msg.size() - (LEVOIT_IDX_CHECKSUM + 1));

            // ---- checksum ----
            levoit_finalize_message(msg);

            return msg;
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
        void Levoit::publish_switch(SwitchType type, bool state)
        {
            auto *sw = switches_[st_idx_(type)];
            if (!sw)
                return;

            // avoid redundant publishes (helps “loop feel”)
            if (sw->state == state)
                return;
            sw->publish_state(state);
        }
        

        void Levoit::publish_sensor(SensorType type, uint32_t value)
        {
            auto *se = sensors_[st_idx_(type)];
            if (!se)
                return;

            float fvalue = static_cast<float>(value);
            // avoid redundant publishes (helps “loop feel”)

            if (se->has_state() && se->state == fvalue)
                return;
            se->publish_state(value);
        }

        void Levoit::publish_number(NumberType type, uint32_t value)
        {
            auto *nm = numbers_[nt_idx_(type)];
            if (!nm)
                return;

            float fvalue = static_cast<float>(value);
            // avoid redundant publishes (helps “loop feel”)

            if (nm->has_state() && nm->state == fvalue)
                return;
            nm->publish_state(value);
        }
        void Levoit::publish_select(SelectType type, uint32_t value)
        {
            auto *sl = selects_[sl_idx_(type)];
            if (!sl)
                return;

            const auto &options = sl->traits.get_options();

            // bounds check: value is an index into options
            if (value >= options.size())
            {
                ESP_LOGW(TAG, "publish_select: invalid index %u for type %d (options=%u)",
                         value, (int)type, (unsigned)options.size());
                return;
            }

            const std::string &opt = options[value];

            // avoid redundant publishes
            if (sl->has_state() && sl->state == opt)
                return;

            sl->publish_state(opt);
        }
        void Levoit::publish_text_sensor(TextSensorType type, const std::string &value)
        {
            auto *tsl = text_sensor_[static_cast<uint8_t>(type)];
            if (!tsl)
                return;

            // avoid redundant publishes
            if (tsl->has_state() && tsl->state == value)
                return;

            tsl->publish_state(value);
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
                // TODO: sendCommand(state ? setWhiteNoiseOn : setWhiteNoiseOff);
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

                this->sendCommand(setTimerMinutes); // in minutes
                break;

            case NumberType::EFFICIENCY_ROOM_SIZE:
                this->sendCommand(setAutoModeEfficient); // takes value from number: Room Size
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
            if (model == "VITAL100S")
                model_ = ModelType::VITAL100S;
        }
        void Levoit::setup()
        {
            ESP_LOGI(TAG, "Setting up Levoit %s", model_ == ModelType::VITAL200S ? "VITAL200S" : "VITAL100S");
            if (model_ == ModelType::VITAL200S)
                cadr = 416;
            if (model_ == ModelType::VITAL100S)
                cadr = 243;
            
        }

        /// @brief The main loop, that is triggeed by the esphome framework automatically
        void Levoit::loop()
        {
            static uint32_t last_check = 0;
            uint32_t now = millis();

            if (this->timer_active_ && now - last_check >= 10000)
            { // 10 seconds
                last_check = now;
                ESP_LOGD(TAG, "Request status - timer");
                // check_something();
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
                             frame[7], frame[8],  frame[1]);
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
            ESP_LOGV(TAG,
                     "RX: type=0x%02X seq=%u size_after_chk=%u cmdSz=%u ptype=%02X%02X",
                     msg_type, seq, size_af, cmd_sz, ptype0, ptype1);

            // Payload starts after the 10-byte header
            const uint8_t *payload = nullptr;
            size_t payload_len = 0;

            if (len > 10)
            {
                payload = msg + 10;
                payload_len = len - 10;
            }

            dispatch_decoder(this, model_, msg_type, seq, cmd_sz, ptype0, ptype1, payload, payload_len);
        }

        /// @brief Sends the commands depending on its type. long method but plain simple.
        /// There are some extra bytes that i did not understand completely, but it works so far :).
        /// @param commandType
        void Levoit::sendCommand(CommandType commandType)
        {
            ESP_LOGD(TAG, "Command triggered: %s", command_type_to_string(commandType));
            std::vector<uint8_t> message;
            // TODO: implement the command sending here based on model!
            switch (commandType)
            {
            case CommandType::setDeviceON:
            {
                message = build_levoit_message({0x02, 0x00, 0x50}, {0x01, 0x01, 0x01});
                break;
            }
            case CommandType::setDeviceOFF:
            {
                message = build_levoit_message({0x02, 0x00, 0x50}, {0x01, 0x01, 0x00});
                break;
            }
            case CommandType::setDeviceFanLvl1:
            {
                message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x01});
                break;
            }
            case CommandType::setDeviceFanLvl2:
            {
                message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x02});
                break;
            }
            case CommandType::setDeviceFanLvl3:
            {
                message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x03});
                break;
            }
            case CommandType::setDeviceFanLvl4:
            {
                message = build_levoit_message({0x02, 0x03, 0x55}, {0x01, 0x01, 0x04});
                break;
            }
            case CommandType::setFanModeAuto:
            {
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x02});
                break;
            }
            case CommandType::setFanModeSleep:
            {
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x01});
                break;
            }
            case CommandType::setFanModeManual:
            {
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x00});
                break;
            }
            case CommandType::setFanModePet:
            {
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x01, 0x01, 0x05});
                break;
            }
            case CommandType::setLightDetectOn:
            {
                message = build_levoit_message({0x02, 0x11, 0x55}, {0x01, 0x01, 0x01});
                break;
            }
            case CommandType::setLightDetectOff:
            {
                message = build_levoit_message({0x02, 0x11, 0x55}, {0x01, 0x01, 0x00});
                break;
            }
            case CommandType::setDisplayOn:
            {
                message = build_levoit_message({0x02, 0x04, 0x55}, {0x01, 0x01, 0x64});
                break;
            }
            case CommandType::setDisplayOff:
            {
                message = build_levoit_message({0x02, 0x04, 0x55}, {0x01, 0x01, 0x00});
                break;
            }
            case CommandType::setDisplayLockOn:
            {
                message = build_levoit_message({0x02, 0x40, 0x51}, {0x01, 0x01, 0x01});
                break;
            }
            case CommandType::setDisplayLockOff:
            {
                message = build_levoit_message({0x02, 0x40, 0x51}, {0x01, 0x01, 0x00});
                break;
            }
            case CommandType::setAutoModeDefault:
            {
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x02, 0x01, 0x00, 0x03, 0x02, 0x00, 0x00});
                break;
            }
            case CommandType::setAutoModeQuiet:
            {
                // set 0x64, 0x00 based on val!
                message = build_levoit_message({0x02, 0x02, 0x55}, {0x02, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00});
                break;
            }
            case CommandType::setAutoModeEfficient:
            {

                // set 0xAD, 0x01 based on val!
                auto *num = this->numbers_[nt_idx_(NumberType::EFFICIENCY_ROOM_SIZE)];
                if (num != nullptr)
                {
                    uint32_t room_size = static_cast<uint32_t>(num->state);

                    uint8_t size_low = room_size & 0xFF;
                    uint8_t size_high = (room_size >> 8) & 0xFF;

                    ESP_LOGD(TAG, ">>> setAutoModeEfficient: room_size=%u size_low=0x%02X size_high=0x%02X",
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
                auto *num = this->numbers_[nt_idx_(NumberType::TIMER)];
                if (num != nullptr)
                {
                    uint32_t secs = static_cast<uint32_t>(num->state) * 60;

                    uint8_t b0 = (secs >> 0)  & 0xFF;
                    uint8_t b1 = (secs >> 8)  & 0xFF;
                    uint8_t b2 = (secs >> 16) & 0xFF;
                    uint8_t b3 = (secs >> 24) & 0xFF;

                    ESP_LOGD(TAG,">>> setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",(unsigned) secs, b0, b1, b2, b3);

                    message = build_levoit_message({0x02, 0x19, 0x50}, {0x01, 0x04, b0,b1,b2,b3});
                    if (secs > 0)
                    {
                        this->start_timer();
                    }
                    else
                    {
                        this->stop_timer();
                    }
                }

                break;
            }
            case CommandType::requestTimerStatus:
            {
                message = build_levoit_message({0x02, 0x1A, 0x50}, {});
                break;
            }
            default:
            {
                ESP_LOGI(TAG, ">>> Command not implemented yet: %s", command_type_to_string(commandType));
                return;
                break;
            }
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
            if (this->model_ == ModelType::VITAL100S || this->model_ == ModelType::VITAL200S)
                pv = 0x02;

            std::vector<uint8_t> message = {0xA5, 0x12, 0xFF, 0x04, 0xFF, 0x00, pv, ptype0, ptype1, 0x00};

            levoit_finalize_message(message);

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
// Force ESPHome/PIO to compile these translation units even if the component build doesn't glob them.
// #include "tlv.cpp"
// #include "vital_status.cpp"
// #include "decoder.cpp"