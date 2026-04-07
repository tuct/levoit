import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_DEVICE_CLASS, CONF_ICON

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitBinarySensor = levoit_ns.class_("LevoitBinarySensor", binary_sensor.BinarySensor, cg.Component)
BinarySensorType = levoit_ns.enum("BinarySensorType")

TYPE_MAP = {
    "filter_low": BinarySensorType.FILTER_LOW,
    "cover_open": BinarySensorType.COVER_OPEN,
}

TYPE_PROPS = {
    "filter_low": {
        CONF_DEVICE_CLASS: "battery",
        CONF_ICON: "mdi:air-filter",
    },
    "cover_open": {
        CONF_DEVICE_CLASS: "door",
        CONF_ICON: "mdi:door-open",
    },
}

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(LevoitBinarySensor).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    var = cg.new_Pvariable(config[CONF_ID])

    bs_type = config[CONF_TYPE]
    config = dict(config)
    for key, val in TYPE_PROPS.get(bs_type, {}).items():
        if key not in config:
            config[key] = val

    await binary_sensor.register_binary_sensor(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_parent(parent))

    bst = TYPE_MAP[bs_type]
    cg.add(var.set_type(bst))
    cg.add(parent.register_binary_sensor(bst, var))
