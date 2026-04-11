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
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
    CONF_MODE,
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
    "filter_lifetime_months": NumberType.FILTER_LIFETIME_MONTHS,
    # Sprout only below
    "led_brightness_min": NumberType.LED_BRIGHTNESS_MIN,
    "led_speed": NumberType.LED_SPEED,
    "white_noise_volume": NumberType.WHITE_NOISE_VOLUME,
    "aqi_scale": NumberType.AQI_SCALE,
}

# (min_value, max_value, step, extra_props)
TYPE_RANGE = {
    "efficiency_room_size": (9.0, 296.0, 1.0, {
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
    "led_brightness_min": (0.0, 100.0, 1.0, {
        CONF_ICON: "mdi:brightness-3",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
    "led_speed": (1.0, 10.0, 1.0, {
        CONF_ICON: "mdi:timer-outline",
        CONF_UNIT_OF_MEASUREMENT: "s",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
    "aqi_scale": (0.0, 500.0, 1.0, {
        CONF_ICON: "mdi:air-filter",
        CONF_UNIT_OF_MEASUREMENT: "µg/m³",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
    "white_noise_volume": (0.0, 255.0, 1.0, {
        CONF_ICON: "mdi:volume-high",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    }),
}

CONFIG_SCHEMA = number.number_schema(LevoitNumber).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
        cv.Optional(CONF_MIN_VALUE): cv.float_,
        cv.Optional(CONF_MAX_VALUE): cv.float_,
        cv.Optional(CONF_STEP): cv.positive_float,
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    ntype = config[CONF_TYPE]
    min_default, max_default, step_default, extra_props = TYPE_RANGE.get(ntype, (0.0, 1000.0, 1.0, {}))
    min_value = config.get(CONF_MIN_VALUE, min_default)
    max_value = config.get(CONF_MAX_VALUE, max_default)
    step = config.get(CONF_STEP, step_default)

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

    if CONF_MODE in config:
        number_ns = cg.esphome_ns.namespace("number")
        NumberMode = number_ns.enum("NumberMode")
        mode_map = {
            "auto":   NumberMode.NUMBER_MODE_AUTO,
            "box":    NumberMode.NUMBER_MODE_BOX,
            "slider": NumberMode.NUMBER_MODE_SLIDER,
        }
        if config[CONF_MODE] in mode_map:
            cg.add(var.set_mode(mode_map[config[CONF_MODE]]))

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[ntype]
    cg.add(var.set_type(st))
    cg.add(parent.register_number(st, var))

