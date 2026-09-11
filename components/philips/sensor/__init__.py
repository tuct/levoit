import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ICON,
    CONF_ACCURACY_DECIMALS,
    CONF_DEVICE_CLASS,
    CONF_STATE_CLASS,
)
from esphome.components.sensor import validate_state_class

from .. import Philips, CONF_PHILIPS_ID, philips_ns

CONF_TYPE = "type"

PhilipsSensor = philips_ns.class_("PhilipsSensor", sensor.Sensor, cg.Component)
SensorType = philips_ns.enum("SensorType", is_class=True)

TYPE_MAP = {
    "filter_clean": SensorType.FILTER_CLEAN,
    "filter_lifetime": SensorType.FILTER_LIFETIME,
    # AC0651 / AC0951 only
    "pm2_5": SensorType.PM2_5,
    "allergen_index": SensorType.ALLERGEN_INDEX,
    # AC0950/AC0951 only
    "timer_remaining": SensorType.TIMER_REMAINING,
}

TYPE_PROPS = {
    "filter_clean": {
        CONF_UNIT_OF_MEASUREMENT: "%",
        CONF_ICON: "mdi:air-filter",
        CONF_ACCURACY_DECIMALS: 0,
    },
    "filter_lifetime": {
        CONF_UNIT_OF_MEASUREMENT: "%",
        CONF_ICON: "mdi:air-filter",
        CONF_ACCURACY_DECIMALS: 0,
    },
    "pm2_5": {
        CONF_UNIT_OF_MEASUREMENT: "µg/m³",
        CONF_DEVICE_CLASS: "pm25",
        CONF_STATE_CLASS: validate_state_class("measurement"),
        CONF_ACCURACY_DECIMALS: 0,
    },
    "allergen_index": {
        CONF_ICON: "mdi:flower-pollen",
        CONF_STATE_CLASS: validate_state_class("measurement"),
        CONF_ACCURACY_DECIMALS: 0,
    },
    "timer_remaining": {
        CONF_UNIT_OF_MEASUREMENT: "min",
        CONF_ICON: "mdi:timer-sand",
        CONF_ACCURACY_DECIMALS: 0,
    },
}

CONFIG_SCHEMA = sensor.sensor_schema(PhilipsSensor).extend(
    {
        cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)


async def to_code(config):
    # Tells philips.cpp this platform is in the build — see the note at
    # the top of philips.cpp on why USE_SENSOR is not sufficient.
    cg.add_define("USE_PHILIPS_SENSOR")
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    stype = config[CONF_TYPE]
    config = dict(config)
    for key, val in TYPE_PROPS.get(stype, {}).items():
        if key not in config:
            config[key] = val
    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)
    cg.add(parent.register_sensor(TYPE_MAP[stype], var))
