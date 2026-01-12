import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitSensor = levoit_ns.class_("LevoitSensor", sensor.Sensor, cg.Component)
SensorType = levoit_ns.enum("SensorType")

TYPE_MAP = {
    "aqi": SensorType.AQI,
    "pm25": SensorType.PM25,
    "timer_current": SensorType.TIMER_CURRENT,
    "efficiency_counter": SensorType.EFFICIENCY_COUNTER,
}
  

CONFIG_SCHEMA = sensor.sensor_schema(LevoitSensor).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)
    await cg.register_component(var, config)

    # parent pointer for write_state -> send UART command
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[config[CONF_TYPE]]
    cg.add(var.set_type(st))
    cg.add(parent.register_sensor(st, var))

