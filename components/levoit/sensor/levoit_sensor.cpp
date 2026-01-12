#include "levoit_sensor.h"
#include "../levoit.h"
#include "esphome/core/entity_base.h"



namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.sensor";

    void LevoitSensor::setup() {
      // pick default
      // this->traits.set_mode(sensor::SensorMode::NUMBER_MODE_AUTO);

      switch (this->type_) {
        case SensorType::EFFICIENCY_COUNTER:
          this->set_device_class("duration");
          this->set_unit_of_measurement("s");
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        case SensorType::TIMER_CURRENT:
          this->set_device_class("duration");
          this->set_unit_of_measurement("min");
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_DIAGNOSTIC);
          break;
        case SensorType::PM25:
          //this->traits.set_mode(sensor::SensorMode::NUMBER_MODE_SLIDER);  // if you want
          this->set_device_class("pm25");
          this->set_unit_of_measurement("µg/m³");
          break;
        case SensorType::AQI:
          //this->traits.set_mode(sensor::SensorMode::NUMBER_MODE_SLIDER);  // if you want
          this->set_device_class("aqi");
          break;
        default:
          break;
      }
    }
    void LevoitSensor::dump_config() { LOG_SENSOR("", "Levoit Sensor", this); }



  } // namespace levoit
} // namespace esphome
