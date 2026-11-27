#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../types.h"
#include "../esphome_ns.h"

namespace esphome {
namespace levoit_component {

class Levoit;

class LevoitBinarySensor : public Component, public binary_sensor::BinarySensor {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_type(BinarySensorType type) { type_ = type; }

 protected:
  Levoit *parent_{nullptr};
  BinarySensorType type_{BinarySensorType::FILTER_LOW};
};

}  // namespace levoit_component
}  // namespace esphome
