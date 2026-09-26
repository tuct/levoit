import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.components.light import LightType
from esphome.const import CONF_OUTPUT_ID

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

LevoitSproutLight = levoit_ns.class_("LevoitSproutLight", light.LightOutput)

CONFIG_SCHEMA = light.light_schema(LevoitSproutLight, LightType.BRIGHTNESS_ONLY).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
    }
)


async def to_code(config):
    # Tells the levoit sources this platform is in the build - see the note
    # at the top of levoit.cpp on why USE_LIGHT is not sufficient.
    cg.add_define("USE_LEVOIT_LIGHT")
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    cg.add(var.set_parent(parent))
    cg.add(parent.register_light(var))
