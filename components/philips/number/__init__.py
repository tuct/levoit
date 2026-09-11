import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID, CONF_UNIT_OF_MEASUREMENT, CONF_ICON, CONF_MODE

from .. import Philips, CONF_PHILIPS_ID, philips_ns

CONF_TYPE = "type"

PhilipsNumber = philips_ns.class_("PhilipsNumber", number.Number, cg.Component)
NumberType = philips_ns.enum("NumberType", is_class=True)

TYPE_MAP = {
    # AC0950/AC0951 only
    "timer": NumberType.TIMER,
}

# The app exposes whole hours, 0 (off) to 12. The wire value is an index
# (hours + 1) — see devices/philips-900-series/README.md.
TYPE_RANGE = {
    "timer": {"min": 0, "max": 12, "step": 1},
}

TYPE_PROPS = {
    "timer": {
        CONF_UNIT_OF_MEASUREMENT: "h",
        CONF_ICON: "mdi:timer-outline",
    },
}

CONFIG_SCHEMA = number.number_schema(PhilipsNumber).extend(
    {
        cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
        cv.Optional(CONF_MODE, default="box"): cv.enum(number.NUMBER_MODES, upper=True),
    }
)


async def to_code(config):
    # Tells philips.cpp this platform is in the build — see the note at
    # the top of philips.cpp on why USE_NUMBER is not sufficient.
    cg.add_define("USE_PHILIPS_NUMBER")
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    ntype = config[CONF_TYPE]
    config = dict(config)
    for key, val in TYPE_PROPS.get(ntype, {}).items():
        if key not in config:
            config[key] = val
    rng = TYPE_RANGE[ntype]
    var = await number.new_number(
        config, min_value=rng["min"], max_value=rng["max"], step=rng["step"]
    )
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(TYPE_MAP[ntype]))
    cg.add(parent.register_number(TYPE_MAP[ntype], var))
