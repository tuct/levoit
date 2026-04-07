import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_ICON, CONF_ENTITY_CATEGORY, ENTITY_CATEGORY_DIAGNOSTIC

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
    "sprout_event": TextSensorType.SPROUT_EVENT,
}

TYPE_PROPS = {
    "mcu_version": {CONF_ICON: "mdi:chip", CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC},
    "esp_version": {CONF_ICON: "mdi:chip", CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC},
    "timer_duration_initial": {CONF_ICON: "mdi:timer"},
    "timer_duration_remaining": {CONF_ICON: "mdi:progress-clock"},
    "auto_mode_room_size_high_fan": {},
    "error_message": {CONF_ICON: "mdi:alert-circle-outline", CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC},
    "sprout_event": {CONF_ICON: "mdi:bell-outline", CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC},
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

    sensor_type = config[CONF_TYPE]
    config = dict(config)
    for key, val in TYPE_PROPS.get(sensor_type, {}).items():
        if key not in config:
            config[key] = val

    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)

    # parent pointer for write_state
    cg.add(var.set_parent(parent))

    # set enum type and register into parent
    st = TYPE_MAP[sensor_type]
    cg.add(var.set_type(st))
    cg.add(parent.register_text_sensor(st, var))

