#pragma once
#include "esphome/components/sensor/sensor.h"
#include "../types.h"

namespace esphome
{
  namespace levoit
  {

    class Levoit;

    class LevoitSensor : public sensor::Sensor, public Component
    {
    public:
      void set_parent(Levoit *parent) { parent_ = parent; }
      void set_type(SensorType t) { type_ = t; }
      void setup() override;
      void dump_config() override;

    protected:
      SensorType type_{SensorType::PM25};
      Levoit *parent_{nullptr};

    };

  } // namespace levoit
} // namespace esphome
