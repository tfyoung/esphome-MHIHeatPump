import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    CONF_MODE,
    CONF_FAN_MODE,
    CONF_SWING_MODE,
)
from esphome.core import CORE, coroutine
from . import MhiHeatPump, CONF_MHIHEATPUMP_ID

DEPENDENCIES = []
AUTO_LOAD = ["climate"]

CONF_SUPPORTS = "supports"
DEFAULT_CLIMATE_MODES = [ 'COOL', 'HEAT', 'DRY', 'FAN_ONLY', ] # 'AUTO',
DEFAULT_FAN_MODES = ['DIFFUSE', 'LOW', 'MEDIUM', 'MIDDLE', 'HIGH', 'AUTO'] # 'AUTO', 
DEFAULT_SWING_MODES = ['OFF', 'VERTICAL']

MHIHeatPump = cg.global_ns.class_("MHIHeatPump", climate.Climate, cg.Component)


CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MHIHeatPump),
        cv.GenerateID(CONF_MHIHEATPUMP_ID): cv.use_id(MhiHeatPump)
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine
def to_code(config):
    paren = yield cg.get_variable(config[CONF_MHIHEATPUMP_ID])
    yield climate.register_climate(paren, config)
