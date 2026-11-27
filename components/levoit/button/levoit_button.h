#pragma once

#include "esphome/core/component.h"
#include "esphome/components/button/button.h"
#include "../types.h"
#include "../esphome_ns.h"

namespace esphome {
namespace levoit_component {

class Levoit;

class LevoitButton : public Component, public button::Button {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(Levoit *parent) { parent_ = parent; }
  void set_type(ButtonType type) { type_ = type; }

 protected:
  void press_action() override;
  Levoit *parent_{nullptr};
  ButtonType type_{ButtonType::RESET_FILTER_STATS};
};

}  // namespace levoit_component
}  // namespace esphome
