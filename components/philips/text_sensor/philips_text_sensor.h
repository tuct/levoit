#pragma once
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "../philips.h"

namespace esphome {
namespace philips {

class PhilipsTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void set_parent(Philips *p) { parent_ = p; }
  void set_type(TextSensorType t) { type_ = t; }
  void dump_config() override;

 protected:
  Philips *parent_{nullptr};
  TextSensorType type_{TextSensorType::MCU_VERSION};
};

}  // namespace philips
}  // namespace esphome
