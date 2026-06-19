import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from .. import Philips, CONF_PHILIPS_ID, philips_ns

CONF_TYPE = "type"

PhilipsTextSensor = philips_ns.class_("PhilipsTextSensor", text_sensor.TextSensor, cg.Component)
TextSensorType = philips_ns.enum("TextSensorType", is_class=True)

TYPE_MAP = {
    "mcu_version": TextSensorType.MCU_VERSION,
}

CONFIG_SCHEMA = text_sensor.text_sensor_schema(PhilipsTextSensor).extend(
    {
        cv.Required(CONF_PHILIPS_ID): cv.use_id(Philips),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PHILIPS_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)
    cg.add(var.set_parent(parent))
    cg.add(var.set_type(TYPE_MAP[config[CONF_TYPE]]))
    cg.add(parent.register_text_sensor(TYPE_MAP[config[CONF_TYPE]], var))
