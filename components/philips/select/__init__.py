import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID

from .. import Philips, CONF_PHILIPS_ID, philips_ns

CONF_TYPE = "type"

PhilipsSelect = philips_ns.class_("PhilipsSelect", select.Select, cg.Component)
SelectType = philips_ns.enum("SelectType", is_class=True)

TYPE_MAP = {
    # AC0950/AC0951 only
    "display_brightness": SelectType.DISPLAY_BRIGHTNESS,
}

# The panel offers exactly these three positions; the wire values behind them
# (0x00 / 0x73 / 0x7B) are opaque constants, so the options are not configurable.
TYPE_OPTIONS = {
    "display_brightness": ["off", "low", "bright"],
}

CONFIG_SCHEMA = select.select_schema(PhilipsSelect).extend(
    {
        cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)


async def to_code(config):
    # Tells philips.cpp this platform is in the build — see the note at
    # the top of philips.cpp on why USE_SELECT is not sufficient.
    cg.add_define("USE_PHILIPS_SELECT")
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    stype = config[CONF_TYPE]
    var = await select.new_select(config, options=TYPE_OPTIONS[stype])
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(TYPE_MAP[stype]))
    cg.add(parent.register_select(TYPE_MAP[stype], var))
