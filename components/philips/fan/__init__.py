import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan

from .. import Philips, CONF_PHILIPS_ID, philips_ns

PhilipsFan = philips_ns.class_("PhilipsFan", cg.Component, fan.Fan)

CONFIG_SCHEMA = (
    fan.fan_schema(PhilipsFan)
    .extend({cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips)})
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    # Tells philips.cpp this platform is in the build — see the note at
    # the top of philips.cpp on why USE_FAN is not sufficient.
    cg.add_define("USE_PHILIPS_FAN")
    var = await fan.new_fan(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_fan(var))
