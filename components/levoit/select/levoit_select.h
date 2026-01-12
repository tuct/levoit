#pragma once
#include "esphome/components/select/select.h"
#include "../types.h"

namespace esphome {
namespace levoit {

class Levoit;

class LevoitSelect :  public select::Select, public Component {
 public:
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_type(SelectType t) { type_ = t; }
  void setup() override;
  void dump_config() override;
  

 protected:
  void control(const std::string &value)override;
  Levoit *parent_{nullptr};
  SelectType type_{SelectType::AUTO_MODE};
};

}  // namespace levoit
}  // namespace esphome
