import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitNumber = levoit_ns.class_("LevoitNumber", number.Number,cg.Component)
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
}
  

CONFIG_SCHEMA = number.number_schema(LevoitNumber).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    min_value = 0.0
    max_value = 1000.0
    step = 1.0



    ntype = config[CONF_TYPE]
#    if ntype == "timer":
#        max_value = 720.0   # max 12 hours
#        step = 30.0         # 30 min steps
#
#    if ntype == "efficiency_room_size":
#        min_value = 132.0   # 9 m²
#        max_value = 792.0   # 56 m² should 54?
#        step = 14           # this is 1 m² in ft²


    var = cg.new_Pvariable(config[CONF_ID])
    # inject into config BEFORE register_number
    config = dict(config)


    await number.register_number(
        var,
        config,
        min_value=min_value,
        max_value=max_value,
        step=step
    )
    await cg.register_component(var, config)

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[ntype]
    cg.add(var.set_type(st))
    cg.add(parent.register_number(st, var))

