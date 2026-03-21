#pragma once
#include <cstdint>

namespace esphome
{
    namespace levoit
    {

        enum ModelType : uint8_t
        {
            VITAL100S = 0,
            VITAL200S = 1,
            CORE300S = 2,
            CORE400S = 3,
            CORE200S = 4,
            CORE600S = 5
        };

        enum class SwitchType : uint8_t
        {
            DISPLAY = 0,
            CHILD_LOCK = 1,
            LIGHT_DETECT = 2,
            QUICK_CLEAN = 3,
            WHITE_NOISE = 4,
            DAYTIME_ENABLED = 5
        };
        // SwitchType aliases (flat namespace)
        static constexpr SwitchType DISPLAY = SwitchType::DISPLAY;
        static constexpr SwitchType CHILD_LOCK = SwitchType::CHILD_LOCK;
        static constexpr SwitchType LIGHT_DETECT = SwitchType::LIGHT_DETECT;
        static constexpr SwitchType QUICK_CLEAN = SwitchType::QUICK_CLEAN;
        static constexpr SwitchType WHITE_NOISE = SwitchType::WHITE_NOISE;
        static constexpr SwitchType DAYTIME_ENABLED = SwitchType::DAYTIME_ENABLED;

        enum class NumberType : uint8_t
        {
            TIMER = 0,
            EFFICIENCY_ROOM_SIZE = 1,
            QUICK_CLEAN_MIN = 2,
            WHITE_NOISE_MIN = 3,
            SLEEP_MODE_MIN = 4,
            FILTER_LIFETIME_MONTHS = 5,
            USED_CADR = 6,
            TOTAL_RUNTIME = 7,
        };
        // NumberType aliases (flat namespace)
        static constexpr NumberType TIMER = NumberType::TIMER;
        static constexpr NumberType EFFICIENCY_ROOM_SIZE = NumberType::EFFICIENCY_ROOM_SIZE;
        static constexpr NumberType QUICK_CLEAN_MIN = NumberType::QUICK_CLEAN_MIN;
        static constexpr NumberType WHITE_NOISE_MIN = NumberType::WHITE_NOISE_MIN;
        static constexpr NumberType SLEEP_MODE_MIN = NumberType::SLEEP_MODE_MIN;
        static constexpr NumberType FILTER_LIFETIME_MONTHS = NumberType::FILTER_LIFETIME_MONTHS;
        static constexpr NumberType USED_CADR = NumberType::USED_CADR;
        static constexpr NumberType TOTAL_RUNTIME = NumberType::TOTAL_RUNTIME;

        enum class SensorType : uint8_t
        {
            AQI = 0,
            PM25 = 1,
            TIMER_CURRENT = 2,
            EFFICIENCY_COUNTER = 3,
            CURRENT_CADR = 4,
            FILTER_LIFE_LEFT = 5,
        };
        // NumberType aliases (flat namespace)
        static constexpr SensorType AQI = SensorType::AQI;
        static constexpr SensorType PM25 = SensorType::PM25;
        static constexpr SensorType TIMER_CURRENT = SensorType::TIMER_CURRENT;
        static constexpr SensorType EFFICIENCY_COUNTER = SensorType::EFFICIENCY_COUNTER;
        static constexpr SensorType CURRENT_CADR = SensorType::CURRENT_CADR;
        static constexpr SensorType FILTER_LIFE_LEFT = SensorType::FILTER_LIFE_LEFT;

        enum class BinarySensorType : uint8_t {
            FILTER_LOW = 0,
        };
        static constexpr BinarySensorType FILTER_LOW = BinarySensorType::FILTER_LOW;

        enum class ButtonType : uint8_t {
            RESET_FILTER_STATS = 0,
        };
        static constexpr ButtonType RESET_FILTER_STATS = ButtonType::RESET_FILTER_STATS;
        
        enum class TextSensorType : uint8_t
        {
            MCU_VERSION = 0,
            ESP_VERSION = 1,    
            TIMER_DURATION_INITIAL = 2,
            TIMER_DURATION_CURRENT = 3,
            AUTO_MODE_ROOM_SIZE_HIGH_FAN = 4,
            ERROR_MESSAGE = 5,

        };
        static constexpr TextSensorType MCU_VERSION = TextSensorType::MCU_VERSION;
        static constexpr TextSensorType ESP_VERSION = TextSensorType::ESP_VERSION;
        static constexpr TextSensorType TIMER_DURATION_INITIAL = TextSensorType::TIMER_DURATION_INITIAL;
        static constexpr TextSensorType TIMER_DURATION_CURRENT = TextSensorType::TIMER_DURATION_CURRENT;
        static constexpr TextSensorType AUTO_MODE_ROOM_SIZE_HIGH_FAN = TextSensorType::AUTO_MODE_ROOM_SIZE_HIGH_FAN;
        static constexpr TextSensorType ERROR_MESSAGE = TextSensorType::ERROR_MESSAGE;



        enum class SelectType : uint8_t
        {
            AUTO_MODE = 0,
            SLEEP_MODE = 1,
            QUICK_CLEAN_FAN_LEVEL = 2,
            WHITE_NOISE_FAN_LEVEL = 3,
            SLEEP_MODE_FAN_MODE_LEVEL = 4,
            DAYTIME_FAN_MODE_LEVEL = 5,
            NIGHTLIGHT = 6,
        };
        static constexpr SelectType AUTO_MODE = SelectType::AUTO_MODE;
        static constexpr SelectType SLEEP_MODE = SelectType::SLEEP_MODE;
        static constexpr SelectType QUICK_CLEAN_FAN_LEVEL = SelectType::QUICK_CLEAN_FAN_LEVEL;
        static constexpr SelectType WHITE_NOISE_FAN_LEVEL = SelectType::WHITE_NOISE_FAN_LEVEL;
        static constexpr SelectType SLEEP_MODE_FAN_MODE_LEVEL = SelectType::SLEEP_MODE_FAN_MODE_LEVEL;
        static constexpr SelectType DAYTIME_FAN_MODE_LEVEL = SelectType::DAYTIME_FAN_MODE_LEVEL;
        static constexpr SelectType NIGHTLIGHT = SelectType::NIGHTLIGHT;



        typedef enum
        {
            ack, //
            setDeviceON,
            setDeviceOFF,
            setDeviceFanLvl1,
            setDeviceFanLvl2,
            setDeviceFanLvl3,
            setDeviceFanLvl4, // not used for core300s
            setDisplayLockOn,
            setDisplayLockOff,
            setDisplayOn,
            setDisplayOff,
            setAutoModeQuiet,
            setAutoModeDefault,
            setAutoModeEfficient,
            setTimerMinutes,
            requestTimerStatus,
            setFanModeManual,
            setFanModeAuto,
            setFanModeSleep,
            resetFilter, // use with care, the filter value is stored in the SC95F8617/ chip and cannot be manipulated
            setWifiLedOn,
            setWifiLedOff,
            setWifiLedBlinking,
            setFilterLedOn,
            setFilterLedOff,
            // dedicated command for setTimer
            // Vitals only below
            setLightDetectOn,
            setLightDetectOff,
            setFanModePet,
            setPowerMode,
            setSleepModeDefault,
            // Core200S nightlight
            setNightlightOff,
            setNightlightMid,
            setNightlightFull,
            COMMAND_TYPE_MAX

            // dedicated command for setSleepModeCustom
        } CommandType;

        static const char *command_type_to_string(CommandType cmd)
        {
            static const char *const names[] = {
                "ack",
                "setDeviceON",
                "setDeviceOFF",
                "setDeviceFanLvl1",
                "setDeviceFanLvl2",
                "setDeviceFanLvl3",
                "setDeviceFanLvl4",
                "setDisplayLockOn",
                "setDisplayLockOff",
                "setDisplayOn",
                "setDisplayOff",
                "setAutoModeQuiet",
                "setAutoModeDefault",
                "setAutoModeEfficient",
                "setTimerMinutes",
                "requestTimerStatus",
                "setFanModeManual",
                "setFanModeAuto",
                "setFanModeSleep",
                "resetFilter",
                "setWifiLedOn",
                "setWifiLedOff",
                "setWifiLedBlinking",
                "setFilterLedOn",
                "setFilterLedOff",
                "setLightDetectOn",
                "setLightDetectOff",
                "setFanModePet",
                "setPowerMode",
                "setSleepModeDefault",
                "setNightlightOff",
                "setNightlightMid",
                "setNightlightFull",
            };
            static_assert(
                sizeof(names) / sizeof(names[0]) == COMMAND_TYPE_MAX,
                "CommandType string table out of sync"
            );
            if (cmd < 0 || cmd >= COMMAND_TYPE_MAX)
                return "UNKNOWN_COMMAND";

            return names[cmd];
        }

    } // namespace levoit
} // namespace esphome
