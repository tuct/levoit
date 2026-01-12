import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitSwitch = levoit_ns.class_("LevoitSwitch", switch.Switch, cg.Component)
SwitchType = levoit_ns.enum("SwitchType")

TYPE_MAP = {
    "display": SwitchType.DISPLAY,
    "child_lock": SwitchType.CHILD_LOCK,
    "light_detect": SwitchType.LIGHT_DETECT,
    "quick_clean": SwitchType.QUICK_CLEAN,
    "white_noise": SwitchType.WHITE_NOISE,
}

CONFIG_SCHEMA = switch.switch_schema(LevoitSwitch).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    var = cg.new_Pvariable(config[CONF_ID])
    await switch.register_switch(var, config)
    await cg.register_component(var, config)

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[config[CONF_TYPE]]
    cg.add(var.set_type(st))
    cg.add(parent.register_switch(st, var))
