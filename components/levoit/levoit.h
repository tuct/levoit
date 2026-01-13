#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/application.h"
#include "types.h"

namespace esphome {
namespace levoit {

// forward declaration 
class LevoitSwitch;  
class LevoitNumber;  
class LevoitFan;
class LevoitSensor;
class LevoitSelect;
class LevoitTextSensor;





class Levoit : public Component, public uart::UARTDevice {
 public:
  const char* get_version() const { return "1.1.0 esphome"; }

  // called from python codegen
  void register_switch(SwitchType type, LevoitSwitch *sw);
  void register_number(NumberType type, LevoitNumber *nm);
  void register_sensor(SensorType type, LevoitSensor *se);
  void register_select(SelectType type, LevoitSelect *sl);
  void register_text_sensor(TextSensorType type, LevoitTextSensor *tsl);
  
  // called by switch, number,... when HA toggles it
  void on_switch_command(SwitchType type, bool state);
  void on_number_command(NumberType type, uint32_t value);  
  void on_select_command(SelectType type, uint32_t value);

  // called by decoder when device reports status
  void publish_switch(SwitchType type, bool state);
  void publish_number(NumberType type, uint32_t value);
  void publish_sensor(SensorType type, uint32_t value);
  void publish_select(SelectType type, uint32_t value);
  void publish_text_sensor(TextSensorType type, const std::string &value);

      
        

 



  // called by fan when HA toggles it
  void on_fan_command(int power, int speed_level, int mode);

  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_device_model(std::string model);
  void process_message(uint8_t *msg, int len);
  void sendCommand(CommandType commandType);   // if CommandType exists in your project
  void ackMessage(uint8_t ptype0, uint8_t ptype1);
  void set_fan(LevoitFan *fan) { this->fan_ = fan; }
  LevoitFan *get_fan() const { return this->fan_; }
  LevoitNumber *get_number(NumberType type) const { return numbers_[nt_idx_(type)]; }
  void start_timer(){this->timer_active_ = true;};
  void stop_timer(){this->timer_active_ = false;}; 
  bool is_timer_active() const { return this->timer_active_; };
  
 
  protected:
  LevoitSwitch *switches_[16] {nullptr};  // enough for your types; or use exact count
  LevoitNumber *numbers_[16] {nullptr};  // enough for your types; or use exact count
  LevoitSensor *sensors_[16] {nullptr};  // enough for your types; or use exact count
  LevoitSelect *selects_[16] {nullptr};  // enough for your types; or use exact count
  LevoitTextSensor *text_sensor_[16] {nullptr};  // enough for your types; or use exact count
  LevoitFan *fan_{nullptr};
  ModelType model_{ModelType::VITAL100S};

  uint16_t cadr = 235;
  uint8_t buffer_[512];
  size_t buf_len_{0};

  bool timer_active_{true};
  // helper to convert enum to index
  static constexpr uint8_t st_idx_(SwitchType t) { return (uint8_t)t; }
  static constexpr uint8_t nt_idx_(NumberType t) { return (uint8_t)t; }
  static constexpr uint8_t st_idx_(SensorType t) { return (uint8_t)t; }
  static constexpr uint8_t sl_idx_(SelectType t) { return (uint8_t)t; }
  static constexpr uint8_t tsl_idx_(TextSensorType t) { return (uint8_t)t; }
};

}  // namespace levoit
}  // namespace esphome