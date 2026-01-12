#pragma once
#include "esphome/components/number/number.h"
#include "../types.h"

namespace esphome {
namespace levoit {

class Levoit;

class LevoitNumber :  public number::Number, public Component {
 public:
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_type(NumberType t) { type_ = t; }
  void setup() override;
  void dump_config() override;
  

 protected:
  void control(float value) override;
  Levoit *parent_{nullptr};
  NumberType type_{NumberType::TIMER};
};

}  // namespace levoit
}  // namespace esphome
