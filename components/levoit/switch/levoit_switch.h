#pragma once
#include "esphome/components/switch/switch.h"
#include "../types.h"

namespace esphome
{
  namespace levoit
  {

    class Levoit;

    class LevoitSwitch : public switch_::Switch, public Component
    {
    public:
      void set_parent(Levoit *parent) { parent_ = parent; }
      void set_type(SwitchType t) { type_ = t; }
      void setup() override;
      void dump_config() override;

    protected:
      void write_state(bool state) override;

      Levoit *parent_{nullptr};
      SwitchType type_{SwitchType::DISPLAY};
    };

  } // namespace levoit
} // namespace esphome
