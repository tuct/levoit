#include "lv600s_status.h"
#include "levoit_device.h"

namespace levoit
{
    // Fixed offsets from devices/levoit-lv600s/uart/panel-mode-timer.csv and PROTOCOL.md.
    void decode_lv600s_status(LevoitDevice *self, const lv600s::Status &status)
    {
        const auto &payload = status.raw;
        const uint8_t raw_mode = payload[11];
        const uint8_t mist = payload[12];
        const uint8_t warm = payload[13];
        const uint8_t fault = payload[15];
        const bool valid_warm = warm <= 3;

        self->publish_text_sensor(TextSensorType::MCU_VERSION,
                                  std::to_string(payload[2]) + "." + std::to_string(payload[1]) + "." +
                                      std::to_string(payload[0]));
        self->publish_switch(SwitchType::DISPLAY, status.display());
        self->publish_binary_sensor(BinarySensorType::MCU_CONNECTION, true);
        self->publish_binary_sensor(BinarySensorType::TANK_REMOVED, status.tank_removed());
        self->publish_binary_sensor(BinarySensorType::WATER_TANK_EMPTY, status.water_empty());
        self->publish_binary_sensor(BinarySensorType::MCU_FAULT, fault != 0);
        self->publish_binary_sensor(BinarySensorType::HUMIDIFYING, status.humidifying());
        self->publish_binary_sensor(BinarySensorType::HEATING_ENABLED, valid_warm && status.heating_enabled());
        self->publish_text_sensor(TextSensorType::ERROR_MESSAGE,
                                  fault ? "MCU fault " + std::to_string(fault) : "No reported MCU fault");
        self->publish_sensor(SensorType::HUMIDITY, payload[9]);
        self->publish_sensor(SensorType::TEMPERATURE, payload[10]);
        self->publish_sensor(SensorType::MIST_LEVEL, status.power() ? (mist >= 1 && mist <= 9 ? float(mist) : NAN) : 0.0f);
        if (valid_warm)
            self->publish_number(NumberType::WARM_LEVEL, warm);
        if (raw_mode != 1 && payload[8] >= 40 && payload[8] <= 80)
            self->publish_number(NumberType::HUMIDITY_TARGET, payload[8]);

        static constexpr const char *reported_modes[]{"Auto (Original)", "Manual", "Sleep", "Humidity"};
        static constexpr int fan_modes[]{-1, 0, 1, 3};
        // Keep Original Auto diagnostic-only. The shared humidifier mode is Humidity.
        const bool known_mode = raw_mode < 4;
        self->publish_text_sensor(TextSensorType::REPORTED_MODE,
                                  known_mode ? reported_modes[raw_mode] : "Unknown");
        self->apply_fan_status(status.power(), mist >= 1 && mist <= 9 ? mist : -1,
                               known_mode ? fan_modes[raw_mode] : -1);
    }
}
