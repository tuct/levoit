import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_DEVICE_CLASS,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ICON,
    CONF_ENTITY_CATEGORY,
    CONF_STATE_CLASS,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from esphome.components.sensor import validate_state_class

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitSensor = levoit_ns.class_("LevoitSensor", sensor.Sensor, cg.Component)
SensorType = levoit_ns.enum("SensorType")

TYPE_MAP = {
    "aqi": SensorType.AQI,
    "pm25": SensorType.PM25,
    "timer_current": SensorType.TIMER_CURRENT,
    "efficiency_counter": SensorType.EFFICIENCY_COUNTER,
    "current_cadr": SensorType.CURRENT_CADR,
    "filter_life_left": SensorType.FILTER_LIFE_LEFT,
    "pm1_0": SensorType.PM1_0,
    "pm10": SensorType.PM10,
    "fan_rpm": SensorType.FAN_RPM,
}

TYPE_PROPS = {
    "efficiency_counter": {
        CONF_DEVICE_CLASS: "duration",
        CONF_UNIT_OF_MEASUREMENT: "s",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
    },
    "timer_current": {
        CONF_DEVICE_CLASS: "duration",
        CONF_UNIT_OF_MEASUREMENT: "min",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
    },
    "pm25": {
        CONF_DEVICE_CLASS: "pm25",
        CONF_UNIT_OF_MEASUREMENT: "µg/m³",
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "aqi": {
        CONF_DEVICE_CLASS: "aqi",
        CONF_ICON: "mdi:molecule",
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "current_cadr": {
        CONF_DEVICE_CLASS: "volume_flow_rate",
        CONF_UNIT_OF_MEASUREMENT: "m³/h",
        CONF_ICON: "mdi:air-filter",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "filter_life_left": {
        CONF_UNIT_OF_MEASUREMENT: "%",
        CONF_ICON: "mdi:air-filter",
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "pm1_0": {
        CONF_DEVICE_CLASS: "pm1",
        CONF_UNIT_OF_MEASUREMENT: "µg/m³",
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "pm10": {
        CONF_DEVICE_CLASS: "pm10",
        CONF_UNIT_OF_MEASUREMENT: "µg/m³",
        CONF_STATE_CLASS: validate_state_class("measurement"),
    },
    "fan_rpm": {
        CONF_ICON: "mdi:fan",
        CONF_UNIT_OF_MEASUREMENT: "RPM",
        CONF_STATE_CLASS: validate_state_class("measurement"),
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
    },
}


CONFIG_SCHEMA = sensor.sensor_schema(LevoitSensor).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    var = cg.new_Pvariable(config[CONF_ID])

    sensor_type = config[CONF_TYPE]
    config = dict(config)
    for key, val in TYPE_PROPS.get(sensor_type, {}).items():
        if key not in config:
            config[key] = val

    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[sensor_type]
    cg.add(var.set_type(st))
    cg.add(parent.register_sensor(st, var))

