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
            CORE600S = 5,
            SPROUT = 6
        };

        enum class SwitchType : uint8_t
        {
            DISPLAY = 0,
            CHILD_LOCK = 1,
            LIGHT_DETECT = 2,
            QUICK_CLEAN = 3,
            WHITE_NOISE = 4,
            DAYTIME_ENABLED = 5,
            LED_RING = 6        // Sprout: decorative LED ring on/off
        };
        // SwitchType aliases (flat namespace)
        static constexpr SwitchType DISPLAY = SwitchType::DISPLAY;
        static constexpr SwitchType CHILD_LOCK = SwitchType::CHILD_LOCK;
        static constexpr SwitchType LIGHT_DETECT = SwitchType::LIGHT_DETECT;
        static constexpr SwitchType QUICK_CLEAN = SwitchType::QUICK_CLEAN;
        static constexpr SwitchType WHITE_NOISE = SwitchType::WHITE_NOISE;
        static constexpr SwitchType DAYTIME_ENABLED = SwitchType::DAYTIME_ENABLED;
        static constexpr SwitchType LED_RING = SwitchType::LED_RING;

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
            // LED_VALUE = 8 — removed, controlled via light component brightness
            LED_BRIGHTNESS_MIN = 9,     // Sprout: breathing mode min brightness (0-100 pct)
            LED_SPEED = 10,             // Sprout: breathing cycle time in seconds (1-10)
            // LED_COLOR_TEMP = 11 — removed, controlled via light component CT slider
            WHITE_NOISE_VOLUME = 12,    // Sprout: white noise volume (0-255)
            AQI_SCALE = 13,             // Sprout: AQI display scale max (0–500)
        };
        // Note: indices 0-11 must stay stable (serialized to preferences)
        // NumberType aliases (flat namespace)
        static constexpr NumberType TIMER = NumberType::TIMER;
        static constexpr NumberType EFFICIENCY_ROOM_SIZE = NumberType::EFFICIENCY_ROOM_SIZE;
        static constexpr NumberType QUICK_CLEAN_MIN = NumberType::QUICK_CLEAN_MIN;
        static constexpr NumberType WHITE_NOISE_MIN = NumberType::WHITE_NOISE_MIN;
        static constexpr NumberType SLEEP_MODE_MIN = NumberType::SLEEP_MODE_MIN;
        static constexpr NumberType FILTER_LIFETIME_MONTHS = NumberType::FILTER_LIFETIME_MONTHS;
        static constexpr NumberType USED_CADR = NumberType::USED_CADR;
        static constexpr NumberType TOTAL_RUNTIME = NumberType::TOTAL_RUNTIME;
        static constexpr NumberType LED_BRIGHTNESS_MIN = NumberType::LED_BRIGHTNESS_MIN;
        static constexpr NumberType LED_SPEED = NumberType::LED_SPEED;
        static constexpr NumberType WHITE_NOISE_VOLUME = NumberType::WHITE_NOISE_VOLUME;
        static constexpr NumberType AQI_SCALE = NumberType::AQI_SCALE;

        enum class SensorType : uint8_t
        {
            AQI = 0,
            PM25 = 1,
            TIMER_CURRENT = 2,
            EFFICIENCY_COUNTER = 3,
            CURRENT_CADR = 4,
            FILTER_LIFE_LEFT = 5,
            PM1_0 = 6,          // Sprout: PM1.0 raw count (tag 0x0C)
            PM10 = 7,           // Sprout: PM10 raw count (tag 0x0D)
            FAN_RPM = 8,        // Sprout: fan tachometer (tag 0x27)
        };
        // SensorType aliases (flat namespace)
        static constexpr SensorType AQI = SensorType::AQI;
        static constexpr SensorType PM25 = SensorType::PM25;
        static constexpr SensorType TIMER_CURRENT = SensorType::TIMER_CURRENT;
        static constexpr SensorType EFFICIENCY_COUNTER = SensorType::EFFICIENCY_COUNTER;
        static constexpr SensorType CURRENT_CADR = SensorType::CURRENT_CADR;
        static constexpr SensorType FILTER_LIFE_LEFT = SensorType::FILTER_LIFE_LEFT;
        static constexpr SensorType PM1_0 = SensorType::PM1_0;
        static constexpr SensorType PM10 = SensorType::PM10;
        static constexpr SensorType FAN_RPM = SensorType::FAN_RPM;

        enum class BinarySensorType : uint8_t {
            FILTER_LOW = 0,
            COVER_OPEN = 1,     // Sprout: cover/filter door open (CMD=02 08 55 tag 0x04)
        };
        static constexpr BinarySensorType FILTER_LOW = BinarySensorType::FILTER_LOW;
        static constexpr BinarySensorType COVER_OPEN = BinarySensorType::COVER_OPEN;

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
            SPROUT_EVENT = 6,   // Sprout: last MCU event string (CMD=02 08 55)
        };
        static constexpr TextSensorType MCU_VERSION = TextSensorType::MCU_VERSION;
        static constexpr TextSensorType ESP_VERSION = TextSensorType::ESP_VERSION;
        static constexpr TextSensorType TIMER_DURATION_INITIAL = TextSensorType::TIMER_DURATION_INITIAL;
        static constexpr TextSensorType TIMER_DURATION_CURRENT = TextSensorType::TIMER_DURATION_CURRENT;
        static constexpr TextSensorType AUTO_MODE_ROOM_SIZE_HIGH_FAN = TextSensorType::AUTO_MODE_ROOM_SIZE_HIGH_FAN;
        static constexpr TextSensorType ERROR_MESSAGE = TextSensorType::ERROR_MESSAGE;
        static constexpr TextSensorType SPROUT_EVENT = TextSensorType::SPROUT_EVENT;



        enum class SelectType : uint8_t
        {
            AUTO_MODE = 0,
            SLEEP_MODE = 1,
            QUICK_CLEAN_FAN_LEVEL = 2,
            WHITE_NOISE_FAN_LEVEL = 3,
            SLEEP_MODE_FAN_MODE_LEVEL = 4,
            DAYTIME_FAN_MODE_LEVEL = 5,
            NIGHTLIGHT = 6,
            LIGHT_MODE = 7,         // Sprout: Off / Nightlight / Breathing
            WHITE_NOISE_SOUND = 8,  // Sprout: white noise sound index (0-14, 15 sounds)
        };
        static constexpr SelectType AUTO_MODE = SelectType::AUTO_MODE;
        static constexpr SelectType SLEEP_MODE = SelectType::SLEEP_MODE;
        static constexpr SelectType QUICK_CLEAN_FAN_LEVEL = SelectType::QUICK_CLEAN_FAN_LEVEL;
        static constexpr SelectType WHITE_NOISE_FAN_LEVEL = SelectType::WHITE_NOISE_FAN_LEVEL;
        static constexpr SelectType SLEEP_MODE_FAN_MODE_LEVEL = SelectType::SLEEP_MODE_FAN_MODE_LEVEL;
        static constexpr SelectType DAYTIME_FAN_MODE_LEVEL = SelectType::DAYTIME_FAN_MODE_LEVEL;
        static constexpr SelectType NIGHTLIGHT = SelectType::NIGHTLIGHT;
        static constexpr SelectType LIGHT_MODE = SelectType::LIGHT_MODE;
        static constexpr SelectType WHITE_NOISE_SOUND = SelectType::WHITE_NOISE_SOUND;



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
            setAutoModeEco,
            setTimerMinutes,
            setTimerStop,       // send 00 00 00 00 (clears timer without touching num->state)
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
            // Sprout LED ring
            setSproutLedOff,            // turn LED ring off (tag01=00)
            setSproutLightNightlight,   // CMD=02 0B 55 — nightlight: brightness pct, CT Kelvin (from light component)
            setSproutLightBreathing,    // CMD=02 0C 55 — breathing: max pct, LED_BRIGHTNESS_MIN, LED_SPEED, CT Kelvin
            // Sprout white noise (CMD=02 07 55 + CMD=02 02 55)
            setSproutWhiteNoiseOn,      // CMD=02 07 55: active=1 (uses WN_VOLUME + WHITE_NOISE_SOUND select)
            setSproutWhiteNoiseOff,     // CMD=02 07 55: active=0
            setSproutWhiteNoiseModeOn,  // CMD=02 02 55: PAY=10 01 01 (enable WN fan mode)
            setSproutWhiteNoiseModeOff, // CMD=02 02 55: PAY=10 01 00 (disable WN fan mode)
            setSproutAqiScale,          // CMD=02 06 55: sets AQI display scale max (0–500)
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
                "setAutoModeEco",
                "setTimerMinutes",
                "setTimerStop",
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
                "setSproutLedOff",
                "setSproutLightNightlight",
                "setSproutLightBreathing",
                "setSproutWhiteNoiseOn",
                "setSproutWhiteNoiseOff",
                "setSproutWhiteNoiseModeOn",
                "setSproutWhiteNoiseModeOff",
                "setSproutAqiScale",
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
