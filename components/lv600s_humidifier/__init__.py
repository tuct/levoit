"""LV600S fixed-layout protocol (9600 8N1), separate from purifier TLVs."""

import importlib.util
from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, binary_sensor, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]
ns = cg.esphome_ns.namespace("lv600s_humidifier")
LV600SHumidifier = ns.class_("LV600SHumidifier", cg.Component, uart.UARTDevice)
SENSORS = [
    "current_humidity",
    "current_temperature",
    "target_humidity",
    "mist_level",
    "warm_level",
    "display_config",
    "mode",
    "fog_status",
    "container_state",
    "status_byte_14",
    "other_exception",
    "timer_remaining",
]
BINARIES = [
    "power",
    "display",
    "water_lacks",
    "tank_removed",
    "humidifying",
    "warm_enabled",
    "online",
    "fault",
]
TEXTS = [
    "mcu_version",
    "command_result",
    "status_raw",
    "fault_message",
    "esp_version",
    "model_sensor",
]


# Same mono-repo version source as levoit and philips, without loading either component.
def repo_version():
    path = Path(__file__).resolve().parent.parent / "levoit" / "version_info.py"
    if not path.is_file():
        return "unknown"
    spec = importlib.util.spec_from_file_location("_lv600s_repo_version", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.version()


fields = {
    cv.GenerateID(): cv.declare_id(LV600SHumidifier),
    cv.Optional("model", default="LV600S"): cv.one_of("LV600S"),
    cv.Optional("status_interval", default="5s"): cv.All(
        cv.positive_time_period_milliseconds,
        cv.Range(
            min=cv.TimePeriod(milliseconds=1000), max=cv.TimePeriod(milliseconds=10000)
        ),
    ),
}
for key in SENSORS:
    kw = {}
    if key in ("current_humidity", "target_humidity"):
        kw = {
            "unit_of_measurement": "%",
            "device_class": "humidity",
            "state_class": "measurement",
        }
    elif key == "current_temperature":
        kw = {
            "unit_of_measurement": "°C",
            "device_class": "temperature",
            "state_class": "measurement",
        }
    elif key == "timer_remaining":
        kw = {"unit_of_measurement": "s", "device_class": "duration"}
    fields[cv.Optional(key)] = sensor.sensor_schema(accuracy_decimals=0, **kw)
for key in BINARIES:
    kw = (
        {"device_class": "connectivity"}
        if key == "online"
        else (
            {"device_class": "problem"}
            if key in ("fault", "water_lacks", "tank_removed")
            else {}
        )
    )
    fields[cv.Optional(key)] = binary_sensor.binary_sensor_schema(**kw)
for key in TEXTS:
    fields[cv.Optional(key)] = text_sensor.text_sensor_schema()
CONFIG_SCHEMA = (
    cv.Schema(fields).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)
)
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "lv600s_humidifier",
    baud_rate=9600,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
    require_tx=True,
    require_rx=True,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add_define("LV600S_VERSION", repo_version())
    cg.add(var.set_status_interval(config["status_interval"].total_milliseconds))
    for keys, module, method in [
        (SENSORS, sensor, var.register_sensor),
        (BINARIES, binary_sensor, var.register_binary_sensor),
        (TEXTS, text_sensor, var.register_text_sensor),
    ]:
        for index, key in enumerate(keys):
            if key in config:
                entity = (
                    await module.new_sensor(config[key])
                    if module == sensor
                    else (
                        await module.new_binary_sensor(config[key])
                        if module == binary_sensor
                        else await module.new_text_sensor(config[key])
                    )
                )
                cg.add(method(index, entity))
