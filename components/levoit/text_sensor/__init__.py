import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_ICON

from .. import Levoit, CONF_LEVOIT_ID, levoit_ns

CONF_TYPE = "type"

LevoitTextSensor = levoit_ns.class_("LevoitTextSensor", text_sensor.TextSensor, cg.Component)
TextSensorType = levoit_ns.enum("TextSensorType")

TYPE_MAP = {
    "mcu_version": TextSensorType.MCU_VERSION,
    "esp_version": TextSensorType.ESP_VERSION,
    "timer_duration_initial": TextSensorType.TIMER_DURATION_INITIAL,
    "timer_duration_remaining": TextSensorType.TIMER_DURATION_CURRENT,
    "auto_mode_room_size_high_fan": TextSensorType.AUTO_MODE_ROOM_SIZE_HIGH_FAN,
    "error_message": TextSensorType.ERROR_MESSAGE,
}

ICON_MAP = {
    "mcu_version": "mdi:chip",
    "esp_version": "mdi:chip",
    "timer_duration_initial": "mdi:timer",
    "timer_duration_remaining": "mdi:progress-clock",
    "auto_mode_room_size_high_fan": None,
    "error_message": "mdi:alert-circle-outline",
}
  

CONFIG_SCHEMA = text_sensor.text_sensor_schema(LevoitTextSensor).extend(
    {
        cv.Required(CONF_LEVOIT_ID): cv.use_id(Levoit),
        cv.Required(CONF_TYPE): cv.one_of(*TYPE_MAP.keys(), lower=True),
        cv.Optional(CONF_ICON): cv.icon,
    }
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_LEVOIT_ID])

    # Set icon in config for the schema to process
    sensor_type = config[CONF_TYPE]
    # Use user-provided icon if available, otherwise use default from ICON_MAP
    if CONF_ICON not in config:
        icon = ICON_MAP.get(sensor_type)
        if icon:
            config[CONF_ICON] = icon

    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)

    # parent pointer for write_state 
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[sensor_type]
    cg.add(var.set_type(st))
    cg.add(parent.register_text_sensor(st, var))
    
    # Also generate C++ code to set icon (for setup() method)
    if icon:
        cg.add(var.set_icon(icon))

