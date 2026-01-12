import esphome.codegen as cg
from esphome.components import fan
from esphome.components.fan import validate_preset_modes
import esphome.config_validation as cv
from esphome.const import CONF_PRESET_MODES, CONF_SPEED_COUNT

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CODEOWNERS = ["@tuct"]

LevoitFan = levoit_ns.class_("LevoitFan", cg.Component, fan.Fan)


CONFIG_SCHEMA = (
    fan.fan_schema(LevoitFan)
    .extend({cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit)})
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):

    var = await fan.new_fan(config)
    await cg.register_component(var, config)
    

    parent = await cg.get_variable(config[CONF_LEVOIT_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_fan(var))

   


