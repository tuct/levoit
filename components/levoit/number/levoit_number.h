#pragma once
#include "esphome/components/number/number.h"
#include "esphome/core/preferences.h"
#include "../types.h"
#include "../esphome_ns.h"

namespace esphome {
namespace levoit_component {

class Levoit;

class LevoitNumber :  public number::Number, public Component {
 public:
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_type(NumberType t) { type_ = t; }
  void setup() override;
  void dump_config() override;
  float normalize_room_size(float value) const;
  

 protected:
  void control(float value) override;
  Levoit *parent_{nullptr};
  NumberType type_{NumberType::TIMER};
  ESPPreferenceObject pref_;
};

}  // namespace levoit_component
}  // namespace esphome
