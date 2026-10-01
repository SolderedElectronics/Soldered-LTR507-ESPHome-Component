import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_AMBIENT_LIGHT,
    CONF_GAIN,
    CONF_ID,
    CONF_INTEGRATION_TIME,
    DEVICE_CLASS_ILLUMINANCE,
    ICON_BRIGHTNESS_5,
    STATE_CLASS_MEASUREMENT,
    UNIT_LUX,
)

DEPENDENCIES = ["i2c"]

CONF_PROXIMITY = "proximity"
CONF_LED_CURRENT = "led_current"
CONF_LED_PULSE_FREQUENCY = "led_pulse_frequency"
CONF_LED_PULSES = "led_pulses"

ICON_PROXIMITY = "mdi:hand-wave-outline"
UNIT_COUNTS = "#"

soldered_ltr507_ns = cg.esphome_ns.namespace("soldered_ltr507")
SolderedLTR507Component = soldered_ltr507_ns.class_(
    "SolderedLTR507Component", cg.PollingComponent, i2c.I2CDevice
)

# ALS_CONTR bits 4:3. Only the two widest ranges are offered: their lux/count factors (1 and 0.5)
# agree across every source, the two narrow ranges do not.
AlsGain = soldered_ltr507_ns.enum("AlsGain")
ALS_GAINS = {
    "1X": AlsGain.ALS_GAIN_1X,
    "2X": AlsGain.ALS_GAIN_2X,
}

# ALS_MEAS_RATE bits 7:5. The 12/8/4-bit modes are left out on purpose.
AlsIntegrationTime = soldered_ltr507_ns.enum("AlsIntegrationTime")
ALS_INTEGRATION_TIMES = {
    "1200ms": AlsIntegrationTime.ALS_INTEGRATION_1200MS,
    "600ms": AlsIntegrationTime.ALS_INTEGRATION_600MS,
    "300ms": AlsIntegrationTime.ALS_INTEGRATION_300MS,
    "150ms": AlsIntegrationTime.ALS_INTEGRATION_150MS,
    "75ms": AlsIntegrationTime.ALS_INTEGRATION_75MS,
}

# PS_LED bits 2:0
PsLedCurrent = soldered_ltr507_ns.enum("PsLedCurrent")
PS_LED_CURRENTS = {
    "5mA": PsLedCurrent.PS_LED_CURRENT_5MA,
    "10mA": PsLedCurrent.PS_LED_CURRENT_10MA,
    "20mA": PsLedCurrent.PS_LED_CURRENT_20MA,
    "50mA": PsLedCurrent.PS_LED_CURRENT_50MA,
    "100mA": PsLedCurrent.PS_LED_CURRENT_100MA,
}

# PS_LED bits 7:5
PsLedPulseFrequency = soldered_ltr507_ns.enum("PsLedPulseFrequency")
PS_LED_PULSE_FREQUENCIES = {
    "30kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_30KHZ,
    "40kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_40KHZ,
    "50kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_50KHZ,
    "60kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_60KHZ,
    "70kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_70KHZ,
    "80kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_80KHZ,
    "90kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_90KHZ,
    "100kHz": PsLedPulseFrequency.PS_LED_PULSE_FREQ_100KHZ,
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SolderedLTR507Component),
            cv.Optional(CONF_AMBIENT_LIGHT): sensor.sensor_schema(
                unit_of_measurement=UNIT_LUX,
                icon=ICON_BRIGHTNESS_5,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_ILLUMINANCE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PROXIMITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                icon=ICON_PROXIMITY,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_GAIN, default="1X"): cv.enum(ALS_GAINS, upper=True),
            cv.Optional(CONF_INTEGRATION_TIME, default="75ms"): cv.enum(
                ALS_INTEGRATION_TIMES, lower=True
            ),
            cv.Optional(CONF_LED_CURRENT, default="50mA"): cv.enum(
                PS_LED_CURRENTS, space=""
            ),
            cv.Optional(CONF_LED_PULSE_FREQUENCY, default="60kHz"): cv.enum(
                PS_LED_PULSE_FREQUENCIES, space=""
            ),
            cv.Optional(CONF_LED_PULSES, default=127): cv.int_range(min=1, max=255),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x3A)),
    cv.has_at_least_one_key(CONF_AMBIENT_LIGHT, CONF_PROXIMITY),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if ambient_light_config := config.get(CONF_AMBIENT_LIGHT):
        sens = await sensor.new_sensor(ambient_light_config)
        cg.add(var.set_ambient_light_sensor(sens))
    if proximity_config := config.get(CONF_PROXIMITY):
        sens = await sensor.new_sensor(proximity_config)
        cg.add(var.set_proximity_sensor(sens))

    cg.add(var.set_gain(config[CONF_GAIN]))
    cg.add(var.set_integration_time(config[CONF_INTEGRATION_TIME]))
    cg.add(var.set_led_current(config[CONF_LED_CURRENT]))
    cg.add(var.set_led_pulse_frequency(config[CONF_LED_PULSE_FREQUENCY]))
    cg.add(var.set_led_pulses(config[CONF_LED_PULSES]))
