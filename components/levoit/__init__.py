import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, switch
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@tuct"]

CONF_LEVOIT_MODEL = "model"
VALID_MODELS = ["VITAL100S", "VITAL200S", "CORE200S", "CORE300S", "CORE400S"] # open: CORE600S, same name: VITAL200S PRO == VITAL200S

CONF_LEVOIT_ID = "levoit" 

levoit_ns = cg.esphome_ns.namespace(CONF_LEVOIT_ID)
Levoit = levoit_ns.class_("Levoit", cg.Component, uart.UARTDevice)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Levoit),
            cv.Optional(CONF_LEVOIT_MODEL, default="VITAL100S"): cv.All(
                cv.string, cv.one_of(*VALID_MODELS)
            ),
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


