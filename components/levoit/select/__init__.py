import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, CONF_OPTIONS, CONF_ENTITY_CATEGORY, ENTITY_CATEGORY_CONFIG


from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitSelect = levoit_ns.class_("LevoitSelect", select.Select, cg.Component)

SelectType = levoit_ns.enum("SelectType")

TYPE_MAP = {
    "auto_mode": SelectType.AUTO_MODE,
    "sleep_mode": SelectType.SLEEP_MODE,
    "quick_clean_fan_level": SelectType.QUICK_CLEAN_FAN_LEVEL,
    "white_noise_fan_level": SelectType.WHITE_NOISE_FAN_LEVEL,
    "sleep_mode_fan_mode_level": SelectType.SLEEP_MODE_FAN_MODE_LEVEL,
    "daytime_fan_mode_level": SelectType.DAYTIME_FAN_MODE_LEVEL,
    "nightlight": SelectType.NIGHTLIGHT,
}

CONFIG_SCHEMA = select.select_schema(LevoitSelect).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    ntype = config[CONF_TYPE]

    config = dict(config)
    if CONF_ENTITY_CATEGORY not in config:
        config[CONF_ENTITY_CATEGORY] = ENTITY_CATEGORY_CONFIG

    await select.register_select(var, config, options=["Default"])
    await cg.register_component(var, config)

    cg.add(var.set_parent(parent))

    st = TYPE_MAP[ntype]
    cg.add(var.set_type(st))
    cg.add(parent.register_select(st, var))

