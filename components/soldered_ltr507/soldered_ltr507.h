/**
 * @file soldered_ltr507.h
 * @brief Public API for the soldered_ltr507 ESPHome component
 * @author Soldered Electronics
 *
 * Driver for the Soldered Digital Light & Proximity Sensor LTR-507 breakout, ported from the Soldered LTR-507 Arduino
 * library and the Soldered LTR507 ESP-IDF component. The LTR-507ALS-01 measures continuously on its own once its ALS
 * and PS blocks are put in active mode; on every update this component reads the latest ambient light value (lux,
 * computed on-chip) and the latest raw 11-bit proximity value.
 */

#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace soldered_ltr507 {

/// ALS_CONTR bits 4:3 (dynamic range)
enum AlsGain : uint8_t {
  ALS_GAIN_1X = 0x00,  // 1 lux/count, up to 64k lux
  ALS_GAIN_2X = 0x01,  // 0.5 lux/count, up to 32k lux
};

/// ALS_MEAS_RATE bits 7:5 (ADC resolution / integration time)
enum AlsIntegrationTime : uint8_t {
  ALS_INTEGRATION_1200MS = 0x00,  // 20-bit
  ALS_INTEGRATION_600MS = 0x01,   // 19-bit
  ALS_INTEGRATION_300MS = 0x02,   // 18-bit
  ALS_INTEGRATION_150MS = 0x03,   // 17-bit
  ALS_INTEGRATION_75MS = 0x04,    // 16-bit, POR default
};

/// PS_LED bits 2:0
enum PsLedCurrent : uint8_t {
  PS_LED_CURRENT_5MA = 0x00,
  PS_LED_CURRENT_10MA = 0x01,
  PS_LED_CURRENT_20MA = 0x02,
  PS_LED_CURRENT_50MA = 0x03,  // POR default
  PS_LED_CURRENT_100MA = 0x04,
};

/// PS_LED bits 7:5
enum PsLedPulseFrequency : uint8_t {
  PS_LED_PULSE_FREQ_30KHZ = 0x00,
  PS_LED_PULSE_FREQ_40KHZ = 0x01,
  PS_LED_PULSE_FREQ_50KHZ = 0x02,
  PS_LED_PULSE_FREQ_60KHZ = 0x03,  // POR default
  PS_LED_PULSE_FREQ_70KHZ = 0x04,
  PS_LED_PULSE_FREQ_80KHZ = 0x05,
  PS_LED_PULSE_FREQ_90KHZ = 0x06,
  PS_LED_PULSE_FREQ_100KHZ = 0x07,
};

class SolderedLTR507Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_ambient_light_sensor(sensor::Sensor *sensor) { this->ambient_light_sensor_ = sensor; }
  void set_proximity_sensor(sensor::Sensor *sensor) { this->proximity_sensor_ = sensor; }
  void set_gain(AlsGain gain) { this->gain_ = gain; }
  void set_integration_time(AlsIntegrationTime integration_time) { this->integration_time_ = integration_time; }
  void set_led_current(PsLedCurrent led_current) { this->led_current_ = led_current; }
  void set_led_pulse_frequency(PsLedPulseFrequency led_pulse_frequency) {
    this->led_pulse_frequency_ = led_pulse_frequency;
  }
  void set_led_pulses(uint8_t led_pulses) { this->led_pulses_ = led_pulses; }

 protected:
  bool configure_();
  void read_ambient_light_(uint8_t status);
  void read_proximity_();

  sensor::Sensor *ambient_light_sensor_{nullptr};
  sensor::Sensor *proximity_sensor_{nullptr};

  AlsGain gain_{ALS_GAIN_1X};
  AlsIntegrationTime integration_time_{ALS_INTEGRATION_75MS};
  PsLedCurrent led_current_{PS_LED_CURRENT_50MA};
  PsLedPulseFrequency led_pulse_frequency_{PS_LED_PULSE_FREQ_60KHZ};
  uint8_t led_pulses_{127};

  uint8_t part_id_{0};
  bool ps_saturated_{false};
};

}  // namespace soldered_ltr507
}  // namespace esphome
