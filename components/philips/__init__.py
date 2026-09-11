import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@tuct"]

CONF_PHILIPS_MODEL = "model"
# 600 series: AC0650 = base (no PM sensor, no Auto), AC0651 = same + PM sensor.
# 900 series: AC0950 / AC0951, same protocol and datapoint numbering, but
# medium fan mode is 0x13 (not 0x01), the HEPA total is 9600 (not 4800), and
# it adds child lock, beep, display brightness and a sleep timer.
# See devices/philips-900-series/README.md.
VALID_MODELS = ["AC0650", "AC0651", "AC0950", "AC0951"]

CONF_PHILIPS_ID = "philips"

philips_ns = cg.esphome_ns.namespace("philips")
Philips = philips_ns.class_("Philips", cg.Component, uart.UARTDevice)
PhilipsModel = philips_ns.enum("PhilipsModel", is_class=True)

MODEL_MAP = {
    "AC0650": PhilipsModel.AC0650,
    "AC0651": PhilipsModel.AC0651,
    "AC0950": PhilipsModel.AC0950,
    "AC0951": PhilipsModel.AC0951,
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Philips),
            cv.Optional(CONF_PHILIPS_MODEL, default="AC0650"): cv.one_of(
                *VALID_MODELS, upper=True
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
    cg.add(var.set_model(MODEL_MAP[config[CONF_PHILIPS_MODEL]]))
