/**
 * @file soldered_ltr507.cpp
 * @brief Implementation of the soldered_ltr507 ESPHome component
 * @author Soldered Electronics
 */

#include "soldered_ltr507.h"

#include "esphome/core/log.h"

namespace esphome {
namespace soldered_ltr507 {

static const char *const TAG = "soldered_ltr507";

static const uint8_t REG_ALS_CONTR = 0x80;
static const uint8_t REG_PS_CONTR = 0x81;
static const uint8_t REG_PS_LED = 0x82;
static const uint8_t REG_PS_N_PULSES = 0x83;
static const uint8_t REG_PS_MEAS_RATE = 0x84;
static const uint8_t REG_ALS_MEAS_RATE = 0x85;
static const uint8_t REG_PART_ID = 0x86;
static const uint8_t REG_MANUFAC_ID = 0x87;
static const uint8_t REG_ALS_DATA_0 = 0x88;  // + ALS_DATA_1 at 0x89, 16-bit
static const uint8_t REG_ALS_PS_STATUS = 0x8A;
static const uint8_t REG_PS_DATA_0 = 0x8B;  // + PS_DATA_1 at 0x8C, 11-bit

static const uint8_t PART_NUMBER = 0x9;  // PART_ID bits 7:4, bits 3:0 are the revision
static const uint8_t MANUFAC_ID = 0x05;

static const uint8_t ALS_CONTR_MODE_ACTIVE = 1 << 1;
static const uint8_t PS_CONTR_MODE_ACTIVE = 1 << 1;
static const uint8_t PS_LED_DUTY_50 = 0x01 << 3;  // bits 4:3, 50% duty is the only value the datasheet allows
static const uint8_t STATUS_ALS_DATA_READY = 1 << 2;
static const uint8_t PS_DATA_1_SATURATED = 1 << 4;
static const uint8_t PS_DATA_1_MASK = 0x07;

/// ALS_MEAS_RATE bits 2:0
static const uint8_t ALS_REPEAT_RATE_500MS = 0x02;  // POR default
static const uint8_t ALS_REPEAT_RATE_1000MS = 0x03;
static const uint8_t ALS_REPEAT_RATE_2000MS = 0x04;
/// PS_MEAS_RATE bits 2:0
static const uint8_t PS_REPEAT_RATE_100MS = 0x03;  // POR default

static float lux_per_count(AlsGain gain) { return gain == ALS_GAIN_2X ? 0.5f : 1.0f; }

static const char *gain_to_str(AlsGain gain) { return gain == ALS_GAIN_2X ? "2X" : "1X"; }

static uint16_t integration_time_ms(AlsIntegrationTime integration_time) {
  switch (integration_time) {
    case ALS_INTEGRATION_1200MS:
      return 1200;
    case ALS_INTEGRATION_600MS:
      return 600;
    case ALS_INTEGRATION_300MS:
      return 300;
    case ALS_INTEGRATION_150MS:
      return 150;
    case ALS_INTEGRATION_75MS:
    default:
      return 75;
  }
}

/// The ALS repeat rate must not be shorter than the integration time; keep the 500 ms POR default unless it is
static uint8_t als_repeat_rate(AlsIntegrationTime integration_time) {
  switch (integration_time) {
    case ALS_INTEGRATION_1200MS:
      return ALS_REPEAT_RATE_2000MS;
    case ALS_INTEGRATION_600MS:
      return ALS_REPEAT_RATE_1000MS;
    default:
      return ALS_REPEAT_RATE_500MS;
  }
}

static const uint8_t PS_LED_CURRENT_MA[] = {5, 10, 20, 50, 100};
static const uint8_t PS_LED_PULSE_FREQ_KHZ[] = {30, 40, 50, 60, 70, 80, 90, 100};

void SolderedLTR507Component::setup() {
  uint8_t manufac_id;
  if (!this->read_byte(REG_PART_ID, &this->part_id_) || !this->read_byte(REG_MANUFAC_ID, &manufac_id)) {
    ESP_LOGE(TAG, "Sensor not responding");
    this->mark_failed();
    return;
  }
  if ((this->part_id_ >> 4) != PART_NUMBER || manufac_id != MANUFAC_ID) {
    ESP_LOGE(TAG, "Unexpected PART_ID 0x%02X / MANUFAC_ID 0x%02X, not an LTR-507", this->part_id_, manufac_id);
    this->mark_failed();
    return;
  }
  if (!this->configure_()) {
    ESP_LOGE(TAG, "Writing configuration failed");
    this->mark_failed();
  }
}

bool SolderedLTR507Component::configure_() {
  // Whole-register writes (no read-modify-write), in the same order as the ESP-IDF component. Each block is only
  // switched to active mode when its sensor is configured, so the IR LED is not pulsed for nothing.
  uint8_t als_meas_rate = (this->integration_time_ << 5) | als_repeat_rate(this->integration_time_);
  uint8_t ps_led = (this->led_pulse_frequency_ << 5) | PS_LED_DUTY_50 | this->led_current_;
  uint8_t als_contr = (this->gain_ << 3) | (this->ambient_light_sensor_ != nullptr ? ALS_CONTR_MODE_ACTIVE : 0);
  uint8_t ps_contr = this->proximity_sensor_ != nullptr ? PS_CONTR_MODE_ACTIVE : 0;

  return this->write_byte(REG_ALS_MEAS_RATE, als_meas_rate) && this->write_byte(REG_PS_LED, ps_led) &&
         this->write_byte(REG_PS_N_PULSES, this->led_pulses_) &&
         this->write_byte(REG_PS_MEAS_RATE, PS_REPEAT_RATE_100MS) && this->write_byte(REG_ALS_CONTR, als_contr) &&
         this->write_byte(REG_PS_CONTR, ps_contr);
}

void SolderedLTR507Component::update() {
  uint8_t status;
  if (!this->read_byte(REG_ALS_PS_STATUS, &status)) {
    ESP_LOGW(TAG, "Reading status failed");
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  if (this->ambient_light_sensor_ != nullptr)
    this->read_ambient_light_(status);
  if (this->proximity_sensor_ != nullptr)
    this->read_proximity_();
}

void SolderedLTR507Component::read_ambient_light_(uint8_t status) {
  // Same validity check as the Arduino library; the bit is clear until the first measurement after wakeup completes
  if ((status & STATUS_ALS_DATA_READY) == 0) {
    ESP_LOGD(TAG, "No new ambient light data yet");
    return;
  }
  uint8_t data[2];
  if (!this->read_bytes(REG_ALS_DATA_0, data, sizeof(data))) {
    ESP_LOGW(TAG, "Reading ambient light failed");
    this->status_set_warning();
    return;
  }
  uint16_t counts = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  float lux = counts * lux_per_count(this->gain_);
  ESP_LOGD(TAG, "Ambient light: %u counts, %.1f lx", counts, lux);
  this->ambient_light_sensor_->publish_state(lux);
}

void SolderedLTR507Component::read_proximity_() {
  uint8_t data[2];
  if (!this->read_bytes(REG_PS_DATA_0, data, sizeof(data))) {
    ESP_LOGW(TAG, "Reading proximity failed");
    this->status_set_warning();
    return;
  }
  uint16_t counts = uint16_t(data[0]) | (uint16_t(data[1] & PS_DATA_1_MASK) << 8);
  bool saturated = (data[1] & PS_DATA_1_SATURATED) != 0;
  if (saturated && !this->ps_saturated_) {
    ESP_LOGW(TAG, "Proximity ADC saturated, readings are unreliable (too close, or strong ambient IR)");
  }
  this->ps_saturated_ = saturated;
  ESP_LOGD(TAG, "Proximity: %u%s", counts, saturated ? " (saturated)" : "");
  this->proximity_sensor_->publish_state(counts);
}

void SolderedLTR507Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Soldered LTR-507:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    return;
  }
  ESP_LOGCONFIG(TAG, "  Part ID: 0x%02X", this->part_id_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Ambient light", this->ambient_light_sensor_);
  if (this->ambient_light_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Gain: %s", gain_to_str(this->gain_));
    ESP_LOGCONFIG(TAG, "    Integration time: %u ms", integration_time_ms(this->integration_time_));
  }
  LOG_SENSOR("  ", "Proximity", this->proximity_sensor_);
  if (this->proximity_sensor_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    LED current: %u mA", PS_LED_CURRENT_MA[this->led_current_]);
    ESP_LOGCONFIG(TAG, "    LED pulse frequency: %u kHz", PS_LED_PULSE_FREQ_KHZ[this->led_pulse_frequency_]);
    ESP_LOGCONFIG(TAG, "    LED pulses: %u", this->led_pulses_);
  }
}

}  // namespace soldered_ltr507
}  // namespace esphome
