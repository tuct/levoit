#pragma once
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"
#include "../philips.h"

namespace esphome {
namespace philips {

// Display brightness (900 series only): "off" / "low" / "bright".
// The three wire values are opaque constants, not a 0-100 scale, which is why
// this is a select rather than a number.
class PhilipsSelect : public select::Select, public Component {
 public:
  void set_parent(Philips *p) { parent_ = p; }
  void set_type(SelectType t) { type_ = t; }
  void dump_config() override;

 protected:
  void control(const std::string &value) override;
  Philips *parent_{nullptr};
  SelectType type_{SelectType::DISPLAY_BRIGHTNESS};
};

}  // namespace philips
}  // namespace esphome
