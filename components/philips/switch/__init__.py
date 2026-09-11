import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import Philips, CONF_PHILIPS_ID, philips_ns

CONF_TYPE = "type"

PhilipsSwitch = philips_ns.class_("PhilipsSwitch", switch.Switch, cg.Component)
SwitchType = philips_ns.enum("SwitchType", is_class=True)

TYPE_MAP = {
    "standby_sensor": SwitchType.STANDBY_SENSOR,
    # AC0950/AC0951 only
    "child_lock": SwitchType.CHILD_LOCK,
    "beep": SwitchType.BEEP,
}

CONFIG_SCHEMA = switch.switch_schema(PhilipsSwitch).extend(
    {
        cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)


async def to_code(config):
    # Tells philips.cpp this platform is in the build — see the note at
    # the top of philips.cpp on why USE_SWITCH is not sufficient.
    cg.add_define("USE_PHILIPS_SWITCH")
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await switch.register_switch(var, config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(TYPE_MAP[config[CONF_TYPE]]))
    cg.add(parent.register_switch(TYPE_MAP[config[CONF_TYPE]], var))
