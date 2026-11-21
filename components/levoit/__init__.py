from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, switch
from esphome.components import time as time_
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@tuct"]

# The component version has one source: the VERSION file next to this one.
# It is compiled in as LEVOIT_VERSION and reported by the esp_version sensor.
LEVOIT_VERSION = (Path(__file__).parent / "VERSION").read_text(encoding="utf-8").strip()

CONF_LEVOIT_MODEL = "model"
VALID_MODELS = ["VITAL100S", "VITAL200S", "CORE200S", "CORE300S", "CORE400S", "CORE600S", "SPROUT", "EVERESTAIR", "SUPERIOR6000S", "ELEVA300X"] # same name: VITAL200S PRO == VITAL200S

CONF_LEVOIT_ID = "levoit"
# Optional clock. The Eleva 300X MCU runs its quiet window itself and asks the
# ESP for the local time (CMD 02 22 55); without a time source it is never set.
CONF_TIME_ID = "time_id"

levoit_ns = cg.esphome_ns.namespace(CONF_LEVOIT_ID)
Levoit = levoit_ns.class_("Levoit", cg.Component, uart.UARTDevice)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Levoit),
            cv.Optional(CONF_LEVOIT_MODEL, default="VITAL100S"): cv.All(
                cv.string, cv.one_of(*VALID_MODELS)
            ),
            cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_device_model(config[CONF_LEVOIT_MODEL]))
    cg.add_define("LEVOIT_VERSION", LEVOIT_VERSION)

    if CONF_TIME_ID in config:
        cg.add_define("USE_LEVOIT_TIME")
        clock = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time(clock))


