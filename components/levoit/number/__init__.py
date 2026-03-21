import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    CONF_DEVICE_CLASS,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ICON,
    CONF_ENTITY_CATEGORY,
    ENTITY_CATEGORY_CONFIG,
)

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitNumber = levoit_ns.class_("LevoitNumber", number.Number, cg.Component)
NumberType = levoit_ns.enum("NumberType")

TYPE_MAP = {
    "timer": NumberType.TIMER,
    "efficiency_room_size": NumberType.EFFICIENCY_ROOM_SIZE,
     #VITALS only below
    "quick_clean_min": NumberType.QUICK_CLEAN_MIN,
    "white_noise_min": NumberType.WHITE_NOISE_MIN,
    "sleep_mode_min": NumberType.SLEEP_MODE_MIN,
    "quick_clean_fan_level": NumberType.QUICK_CLEAN_FAN_LEVEL,
    "white_noise_fan_level": NumberType.WHITE_NOISE_FAN_LEVEL,
    "sleep_mode_fan_mode_level": NumberType.SLEEP_MODE_FAN_MODE_LEVEL,
    "filter_lifetime_months": NumberType.FILTER_LIFETIME_MONTHS,
}

# (min_value, max_value, step, extra_props)
TYPE_RANGE = {
    "efficiency_room_size": (132.0, 792.0, 14.0, {
        CONF_DEVICE_CLASS: "area",
        CONF_UNIT_OF_MEASUREMENT: "m²",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
    "timer": (0.0, 720.0, 30.0, {
        CONF_DEVICE_CLASS: "duration",
        CONF_UNIT_OF_MEASUREMENT: "min",
    }),
    "filter_lifetime_months": (1.0, 12.0, 1.0, {
        CONF_UNIT_OF_MEASUREMENT: "months",
        CONF_ICON: "mdi:air-filter",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
}

CONFIG_SCHEMA = number.number_schema(LevoitNumber).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    ntype = config[CONF_TYPE]
    min_value, max_value, step, extra_props = TYPE_RANGE.get(ntype, (0.0, 1000.0, 1.0, {}))

    var = cg.new_Pvariable(config[CONF_ID])
    config = dict(config)
    for key, val in extra_props.items():
        if key not in config:
            config[key] = val

    await number.register_number(
        var,
        config,
        min_value=min_value,
        max_value=max_value,
        step=step,
    )
    await cg.register_component(var, config)

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[ntype]
    cg.add(var.set_type(st))
    cg.add(parent.register_number(st, var))

