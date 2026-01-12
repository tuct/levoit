#pragma once
#include "esphome/components/text_sensor/text_sensor.h"
#include "../types.h"

namespace esphome
{
  namespace levoit
  {

    class Levoit;

    class LevoitTextSensor : public text_sensor::TextSensor, public Component
    {
    public:
      void set_parent(Levoit *parent) { parent_ = parent; }
      void set_type(TextSensorType t) { type_ = t; }
      void setup() override;
      void dump_config() override;

    protected:
      TextSensorType type_{TextSensorType::MCU_VERSION};
      Levoit *parent_{nullptr};

    };

  } // namespace levoit
} // namespace esphome
