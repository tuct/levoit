#include "sen6x.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include <cinttypes>
#include <functional>
#include <memory>

namespace esphome {
namespace sen6x {

static const char *const TAG = "sen6x";

//static const uint16_t SEN6X_CMD_AUTO_CLEANING_INTERVAL = 0x8004; //not used with Sen6X
static const uint16_t SEN6X_CMD_GET_DATA_READY_STATUS = 0x0202;
static const uint16_t SEN6X_CMD_GET_FIRMWARE_VERSION = 0xD100; 
static const uint16_t SEN6X_CMD_GET_PRODUCT_NAME = 0xD014;  
static const uint16_t SEN6X_CMD_GET_SERIAL_NUMBER = 0xD033;
static const uint16_t SEN6X_CMD_GET_DEVICE_STATUS = 0xD206;
static const uint16_t SEN6X_CMD_GET_AND_CLEAR_DEVICE_STATUS = 0xD210;
static const uint16_t SEN6X_CMD_NOX_ALGORITHM_TUNING = 0x60E1;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT = 0x0300; //SEN66 only!
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_SEN62 = 0x04A3;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_SEN63C = 0x0471;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_SEN65 = 0x0446;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_SEN68 = 0x0467;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_SEN69C = 0x04B5;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_RAW_SEN62_SEN63C = 0x0492;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_RAW_SEN65_SEN68_SEN69C = 0x0455;
static const uint16_t SEN6X_CMD_READ_MEASUREMENT_RAW_SEN66 = 0x0405;
static const uint16_t SEN6X_CMD_READ_NUMBER_CONCENTRATION = 0x0316;
static const uint16_t SEN6X_CMD_RHT_ACCELERATION_MODE = 0x6100; // Set Temperature Acceleration Parameters
static const uint16_t SEN6X_CMD_START_CLEANING_FAN = 0x5607;
static const uint16_t SEN6X_CMD_START_MEASUREMENTS = 0x0021;
static const uint16_t SEN6X_CMD_START_MEASUREMENTS_RHT_ONLY = 0x0037; // not in SEN6x datasheet
static const uint16_t SEN6X_CMD_STOP_MEASUREMENTS = 0x0104;
static const uint16_t SEN6X_CMD_TEMPERATURE_COMPENSATION = 0x60B2;
static const uint16_t SEN6X_CMD_VOC_ALGORITHM_STATE = 0x6181;
static const uint16_t SEN6X_CMD_VOC_ALGORITHM_TUNING = 0x60D0;
static const uint16_t SEN6X_CMD_SHT_HEATER_ACTIVATE = 0x6765;
static const uint16_t SEN6X_CMD_SHT_HEATER_MEASUREMENTS = 0x6790;
static const uint16_t SEN6X_CMD_CO2_FORCE_RECALIBRATION = 0x6707;
static const uint16_t SEN6X_CMD_CO2_SENSOR_FACTORY_RESET = 0x6754;
static const uint16_t SEN6X_CMD_CO2_SENSOR_AUTOMATIC_SELF_CAL = 0x6711;
static const uint16_t SEN6X_CMD_AMBIENT_PRESSURE = 0x6720;
static const uint16_t SEN6X_CMD_SENSOR_ALTITUDE = 0x6736;
static const uint16_t SEN6X_CMD_RESET = 0xD304;

void SEN6XComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up sen6x...");

  // the sensor needs 100 ms to enter the idle state
  this->set_timeout(500, [this]() {

    // Check if measurement is ready before reading the value
    if (!this->write_command(SEN6X_CMD_GET_DATA_READY_STATUS)) {
      ESP_LOGE(TAG, "Failed to write data ready status command");
      this->mark_failed();
      return;
    }

    uint16_t raw_read_status;
    if (!this->read_data(raw_read_status)) {
      ESP_LOGE(TAG, "Failed to read data ready status");
      this->mark_failed();
      return;
    }
    
    uint32_t stop_measurement_delay = 0;
    // In order to query the device periodic measurement must be ceased => use reset!
    if (raw_read_status) {
      ESP_LOGD(TAG, "Sensor has data available, stopping periodic measurement / reset");
      //if (!this->write_command(SEN6X_CMD_RESET)) {
      if (!this->write_command(SEN6X_CMD_STOP_MEASUREMENTS)) {
        ESP_LOGE(TAG, "Failed to stop measurements ");
        this->mark_failed();
        return;
      }
      stop_measurement_delay = 1400;
    }
    this->set_timeout(stop_measurement_delay, [this]() {
      uint16_t raw_serial_number[3];
      if (!this->get_register(SEN6X_CMD_GET_SERIAL_NUMBER, raw_serial_number, 3, 20)) {
        ESP_LOGE(TAG, "Failed to read serial number");
        this->error_code_ = SERIAL_NUMBER_IDENTIFICATION_FAILED;
        this->mark_failed();
        return;
      }
      this->serial_number_[0] = static_cast<bool>(uint16_t(raw_serial_number[0]) & 0xFF);
      this->serial_number_[1] = static_cast<uint16_t>(raw_serial_number[0] & 0xFF);
      this->serial_number_[2] = static_cast<uint16_t>(raw_serial_number[1] >> 8);
      ESP_LOGD(TAG, "Serial number %02d.%02d.%02d", serial_number_[0], serial_number_[1], serial_number_[2]);

      uint16_t raw_product_name[16];

      if (!this->get_register(SEN6X_CMD_GET_PRODUCT_NAME, raw_product_name, 16, 20)) {
        ESP_LOGE(TAG, "Failed to read product name");
        this->error_code_ = PRODUCT_NAME_FAILED;
        this->mark_failed();
        return;
      }

      // 2 ASCII bytes are encoded in an int
      const uint16_t *current_int = raw_product_name;
      char current_char;
      uint8_t max = 16;
      do {
        // first char
        current_char = *current_int >> 8;
        if (current_char) {
          product_name_.push_back(current_char);
          // second char
          current_char = *current_int & 0xFF;
          if (current_char)
            product_name_.push_back(current_char);
        }
        current_int++;
      } while (current_char && --max);

      this->sen6x_type_ = UNKNOWN;
      if (product_name_ == "SEN62") {
        this->sen6x_type_ = SEN62;
      } else if (product_name_ == "SEN63C") {
        this->sen6x_type_ = SEN63C;
      } else if (product_name_ == "SEN65") {
        this->sen6x_type_ = SEN65;
      } else if (product_name_ == "SEN66") {
        this->sen6x_type_ = SEN66;
      } else if (product_name_ == "SEN68") {
        this->sen6x_type_ = SEN68;
      } else if (product_name_ == "SEN69C") {
        this->sen6x_type_ = SEN69C;
      } else if (product_name_ == "") { // empty name
        ESP_LOGD(TAG, "Productname empty, falling back to SEN66");
        this->sen6x_type_ = SEN66;
      }
      ESP_LOGD(TAG, "Productname %s", product_name_.c_str());


      if (!this->get_register(SEN6X_CMD_GET_FIRMWARE_VERSION, this->firmware_version_, 20)) {
        ESP_LOGE(TAG, "Failed to read firmware version");
        this->error_code_ = FIRMWARE_FAILED;
        this->mark_failed();
        return;
      }
      this->firmware_version_ >>= 8;
      ESP_LOGD(TAG, "Firmware version %d", this->firmware_version_);

      if (this->voc_sensor_ && this->store_baseline_) {
        uint32_t combined_serial =
            encode_uint24(this->serial_number_[0], this->serial_number_[1], this->serial_number_[2]);
        // Hash with compilation time and serial number
        // This ensures the baseline storage is cleared after OTA
        // Serial numbers are unique to each sensor, so mulitple sensors can be used without conflict
        char build_time[App.BUILD_TIME_STR_SIZE] = {0};
        App.get_build_time_string(build_time);
        uint32_t hash = fnv1_hash(std::string(build_time) + std::to_string(combined_serial));
        this->pref_ = global_preferences->make_preference<Sen6xBaselines>(hash, true);

        if (this->pref_.load(&this->voc_baselines_storage_)) {
          ESP_LOGI(TAG, "Loaded VOC baseline state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
        }

        // Initialize storage timestamp
        this->seconds_since_last_store_ = 0;

        if (this->voc_baselines_storage_.state0 > 0 && this->voc_baselines_storage_.state1 > 0) {
          ESP_LOGI(TAG, "Setting VOC baseline from save state0: 0x%04" PRIX32 ", state1: 0x%04" PRIX32,
                   this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
          uint16_t states[4];

          states[0] = voc_baselines_storage_.state0 >> 16;
          states[1] = voc_baselines_storage_.state0 & 0xFFFF;
          states[2] = voc_baselines_storage_.state1 >> 16;
          states[3] = voc_baselines_storage_.state1 & 0xFFFF;

          if (!this->write_command(SEN6X_CMD_VOC_ALGORITHM_STATE, states, 4)) {
            ESP_LOGE(TAG, "Failed to set VOC baseline from saved state");
          }
        }
      }
      if (this->voc_tuning_params_.has_value()) {
        this->write_tuning_parameters_(SEN6X_CMD_VOC_ALGORITHM_TUNING, this->voc_tuning_params_.value());
        delay(20);
      }
      if (this->nox_tuning_params_.has_value()) {
        this->write_tuning_parameters_(SEN6X_CMD_NOX_ALGORITHM_TUNING, this->nox_tuning_params_.value());
        delay(20);
      }

      if (this->temperature_compensation_.has_value()) {
        this->write_temperature_compensation_(this->temperature_compensation_.value());
        delay(20);
      }
      if (this->temperature_acceleration_.has_value()) {
        this->write_temperature_acceleration_(this->temperature_acceleration_.value());
        delay(20);
      }

      // Finally start sensor measurements
      auto cmd = SEN6X_CMD_START_MEASUREMENTS_RHT_ONLY;
      if (this->pm_1_0_sensor_ || this->pm_2_5_sensor_ || this->pm_4_0_sensor_ || this->pm_10_0_sensor_) {
        // if any of the gas sensors are active we need a full measurement
        cmd = SEN6X_CMD_START_MEASUREMENTS;
      }

      if (!this->write_command(cmd)) {
        ESP_LOGE(TAG, "Error starting continuous measurements.");
        this->error_code_ = MEASUREMENT_INIT_FAILED;
        this->mark_failed();
        return;
      }
      initialized_ = true;
      ESP_LOGD(TAG, "Sensor initialized");
    });
  });
}

void SEN6XComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "sen6x:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    switch (this->error_code_) {
      case COMMUNICATION_FAILED:
        ESP_LOGW(TAG, "Communication failed! Is the sensor connected?");
        break;
      case MEASUREMENT_INIT_FAILED:
        ESP_LOGW(TAG, "Measurement Initialization failed!");
        break;
      case SERIAL_NUMBER_IDENTIFICATION_FAILED:
        ESP_LOGW(TAG, "Unable to read sensor serial id");
        break;
      case PRODUCT_NAME_FAILED:
        ESP_LOGW(TAG, "Unable to read product name");
        break;
      case FIRMWARE_FAILED:
        ESP_LOGW(TAG, "Unable to read sensor firmware version");
        break;
      default:
        ESP_LOGW(TAG, "Unknown setup error!");
        break;
    }
  }
  ESP_LOGCONFIG(TAG, "  Productname: %s", this->product_name_.c_str());
  ESP_LOGCONFIG(TAG, "  Firmware version: %d", this->firmware_version_);
  ESP_LOGCONFIG(TAG, "  Serial number %02d.%02d.%02d", serial_number_[0], serial_number_[1], serial_number_[2]);

  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "PM  1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM  2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM  4.0", this->pm_4_0_sensor_);
  LOG_SENSOR("  ", "PM 10.0", this->pm_10_0_sensor_);
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "VOC", this->voc_sensor_);
  LOG_SENSOR("  ", "NOx", this->nox_sensor_);
  LOG_SENSOR("  ", "HCHO", this->hcho_sensor_);
  LOG_SENSOR("  ", "CO2", this->co2_sensor_);  // SEN66

  if (this->temperature_compensation_.has_value()) {
    const auto &comp = this->temperature_compensation_.value();
    ESP_LOGCONFIG(TAG, "  Temperature compensation: offset=%.2fC slope=%.4f time_constant=%us slot=%u",
                 comp.offset / 200.0f, comp.normalized_offset_slope / 10000.0f, comp.time_constant, comp.slot);
  }
  if (this->temperature_acceleration_.has_value()) {
    const auto &accel = this->temperature_acceleration_.value();
    ESP_LOGCONFIG(TAG, "  Temperature acceleration: K=%.1f P=%.1f T1=%.1fs T2=%.1fs",
                 accel.k / 10.0f, accel.p / 10.0f, accel.t1 / 10.0f, accel.t2 / 10.0f);
  }
}

void SEN6XComponent::update() {
  if (!initialized_) {
    return;
  }
  // Store baselines after defined interval or if the difference between current and stored baseline becomes too
  // much
  if (this->store_baseline_ && this->seconds_since_last_store_ > SHORTEST_BASELINE_STORE_INTERVAL) {
    if (this->write_command(SEN6X_CMD_VOC_ALGORITHM_STATE)) {
      // run it a bit later to avoid adding a delay here
      this->set_timeout(550, [this]() {
        uint16_t states[4];
        if (this->read_data(states, 4)) {
          uint32_t state0 = states[0] << 16 | states[1];
          uint32_t state1 = states[2] << 16 | states[3];
          if ((uint32_t) std::abs(static_cast<int32_t>(this->voc_baselines_storage_.state0 - state0)) >
                  MAXIMUM_STORAGE_DIFF ||
              (uint32_t) std::abs(static_cast<int32_t>(this->voc_baselines_storage_.state1 - state1)) >
                  MAXIMUM_STORAGE_DIFF) {
            this->seconds_since_last_store_ = 0;
            this->voc_baselines_storage_.state0 = state0;
            this->voc_baselines_storage_.state1 = state1;

            if (this->pref_.save(&this->voc_baselines_storage_)) {
              ESP_LOGI(TAG, "Stored VOC baseline state0: 0x%04" PRIX32 " ,state1: 0x%04" PRIX32,
                       this->voc_baselines_storage_.state0, voc_baselines_storage_.state1);
            } else {
              ESP_LOGW(TAG, "Could not store VOC baselines");
            }
          }
        }
      });
    }
  }



  uint16_t read_cmd = SEN6X_CMD_READ_MEASUREMENT;
  uint8_t read_words = 9;
  switch (this->sen6x_type_) {
    case SEN62:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT_SEN62;
      read_words = 6;
      break;
    case SEN63C:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT_SEN63C;
      read_words = 7;
      break;
    case SEN65:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT_SEN65;
      read_words = 8;
      break;
    case SEN66:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT;
      read_words = 9;
      break;
    case SEN68:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT_SEN68;
      read_words = 9;
      break;
    case SEN69C:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT_SEN69C;
      read_words = 10;
      break;
    default:
      read_cmd = SEN6X_CMD_READ_MEASUREMENT;
      read_words = 9;
      break;
  }

  const uint8_t poll_retries = 24;
  auto poll_ready = std::make_shared<std::function<void(uint8_t)>>();
  *poll_ready = [this, poll_ready, poll_retries, read_cmd, read_words](uint8_t retries_left) {
    const uint8_t attempt = static_cast<uint8_t>(poll_retries - retries_left + 1);
    ESP_LOGV(TAG, "Data ready polling attempt %u", attempt);
    if (!this->write_command(SEN6X_CMD_GET_DATA_READY_STATUS)) {
      this->status_set_warning();
      ESP_LOGD(TAG, "write error data ready status (%d)", this->last_error_);
      return;
    }

    uint16_t raw_read_status;
    if (!this->read_data(raw_read_status)) {
      this->status_set_warning();
      ESP_LOGD(TAG, "read data ready status error (%d)", this->last_error_);
      return;
    }

    if ((raw_read_status & 0x0001) == 0) {
      if (retries_left == 0) {
        this->status_set_warning();
        ESP_LOGD(TAG, "data not ready in time");
        return;
      }
      this->set_timeout(50, [this, poll_ready, retries_left]() { (*poll_ready)(retries_left - 1); });
      return;
    }

    if (!this->write_command(read_cmd)) {
      this->status_set_warning();
      ESP_LOGD(TAG, "write error read measurement (%d)", this->last_error_);
      return;
    }

    this->set_timeout(20, [this, read_words]() {
      uint16_t measurements[10];

      if (!this->read_data(measurements, read_words)) {
        this->status_set_warning();
        ESP_LOGD(TAG, "read data error (%d)", this->last_error_);
        return;
      }
      int8_t voc_index = -1;
      int8_t nox_index = -1;
      int8_t hcho_index = -1;
      int8_t co2_index = -1;
      bool co2_uint16 = false;
      switch (this->sen6x_type_) {
        case SEN62:
          break;
        case SEN63C:
          co2_index = 6;
          break;
        case SEN65:
          voc_index = 6;
          nox_index = 7;
          break;
        case SEN66:
          voc_index = 6;
          nox_index = 7;
          co2_index = 8;
          co2_uint16 = true;
          break;
        case SEN68:
          voc_index = 6;
          nox_index = 7;
          hcho_index = 8;
          break;
        case SEN69C:
          voc_index = 6;
          nox_index = 7;
          hcho_index = 8;
          co2_index = 9;
          break;
        default:
          break;
      }

      float pm_1_0 = measurements[0] / 10.0f;
      if (measurements[0] == 0xFFFF)
        pm_1_0 = NAN;
      float pm_2_5 = measurements[1] / 10.0f;
      if (measurements[1] == 0xFFFF)
        pm_2_5 = NAN;
      float pm_4_0 = measurements[2] / 10.0f;
      if (measurements[2] == 0xFFFF)
        pm_4_0 = NAN;
      float pm_10_0 = measurements[3] / 10.0f;
      if (measurements[3] == 0xFFFF)
        pm_10_0 = NAN;
      float humidity = static_cast<int16_t>(measurements[4]) / 100.0f;
      if (measurements[4] == 0x7FFF)
        humidity = NAN;
      float temperature = static_cast<int16_t>(measurements[5]) / 200.0f;
      if (measurements[5] == 0x7FFF)
        temperature = NAN;

      float voc = NAN;
      float nox = NAN;
      float hcho = NAN;
      float co2 = NAN;

      if (voc_index >= 0) {
        voc = static_cast<int16_t>(measurements[voc_index]) / 10.0f;
        if (measurements[voc_index] == 0x7FFF)
          voc = NAN;
      }
      if (nox_index >= 0) {
        nox = static_cast<int16_t>(measurements[nox_index]) / 10.0f;
        if (measurements[nox_index] == 0x7FFF)
          nox = NAN;
      }

      if (hcho_index >= 0) {
        const uint16_t hcho_raw = measurements[hcho_index];
        hcho = hcho_raw / 10.0f;
        if (hcho_raw == 0xFFFF)
          hcho = NAN;
      }

      if (co2_index >= 0) {
        if (co2_uint16) {
          const uint16_t co2_raw = measurements[co2_index];
          co2 = static_cast<float>(co2_raw);
          if (co2_raw == 0xFFFF)
            co2 = NAN;
        } else {
          const int16_t co2_raw = static_cast<int16_t>(measurements[co2_index]);
          co2 = static_cast<float>(co2_raw);
          if (co2_raw == 0x7FFF)
            co2 = NAN;
        }
      }

      if (this->pm_1_0_sensor_ != nullptr)
        this->pm_1_0_sensor_->publish_state(pm_1_0);
      if (this->pm_2_5_sensor_ != nullptr)
        this->pm_2_5_sensor_->publish_state(pm_2_5);
      if (this->pm_4_0_sensor_ != nullptr)
        this->pm_4_0_sensor_->publish_state(pm_4_0);
      if (this->pm_10_0_sensor_ != nullptr)
        this->pm_10_0_sensor_->publish_state(pm_10_0);
      if (this->temperature_sensor_ != nullptr)
        this->temperature_sensor_->publish_state(temperature);
      if (this->humidity_sensor_ != nullptr)
        this->humidity_sensor_->publish_state(humidity);
      if (this->voc_sensor_ != nullptr && voc_index >= 0)
        this->voc_sensor_->publish_state(voc);
      if (this->nox_sensor_ != nullptr && nox_index >= 0)
        this->nox_sensor_->publish_state(nox);
      if (this->hcho_sensor_ != nullptr && hcho_index >= 0)
        this->hcho_sensor_->publish_state(hcho);
      if (this->co2_sensor_ != nullptr && co2_index >= 0)
        this->co2_sensor_->publish_state(co2);
      this->status_clear_warning();
    });
  };

  (*poll_ready)(poll_retries);
}

bool SEN6XComponent::write_tuning_parameters_(uint16_t i2c_command, const GasTuning &tuning) {
  uint16_t params[6];
  params[0] = tuning.index_offset;
  params[1] = tuning.learning_time_offset_hours;
  params[2] = tuning.learning_time_gain_hours;
  params[3] = tuning.gating_max_duration_minutes;
  params[4] = tuning.std_initial;
  params[5] = tuning.gain_factor;
  auto result = write_command(i2c_command, params, 6);
  if (!result) {
    ESP_LOGE(TAG, "set tuning parameters failed. i2c command=%0xX, err=%d", i2c_command, this->last_error_);
  }
  return result;
}

bool SEN6XComponent::write_temperature_compensation_(const TemperatureCompensation &compensation) {
  uint16_t params[4];
  params[0] = compensation.offset;
  params[1] = compensation.normalized_offset_slope;
  params[2] = compensation.time_constant;
  params[3] = compensation.slot;
  if (!write_command(SEN6X_CMD_TEMPERATURE_COMPENSATION, params, 4)) {
    ESP_LOGE(TAG, "set temperature_compensation failed. Err=%d", this->last_error_);
    return false;
  }
  return true;
}

bool SEN6XComponent::write_temperature_acceleration_(const TemperatureAcceleration &acceleration) {
  uint16_t params[4];
  params[0] = acceleration.k;
  params[1] = acceleration.p;
  params[2] = acceleration.t1;
  params[3] = acceleration.t2;
  if (!write_command(SEN6X_CMD_RHT_ACCELERATION_MODE, params, 4)) {
    ESP_LOGE(TAG, "set temperature_acceleration failed. Err=%d", this->last_error_);
    return false;
  }
  return true;
}

bool SEN6XComponent::start_fan_cleaning() {
  if (!write_command(SEN6X_CMD_START_CLEANING_FAN)) {
    this->status_set_warning();
    ESP_LOGE(TAG, "write error start fan (%d)", this->last_error_);
    return false;
  } else {
    ESP_LOGD(TAG, "Fan auto clean started");
  }
  return true;
}

}  // namespace sen6x
}  // namespace esphome
