#include "levoit_sensor.h"
#include "../levoit.h"



namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.sensor";

    void LevoitSensor::setup() {
      switch (this->type_) {
        case SensorType::FILTER_LIFE_LEFT:
          this->set_accuracy_decimals(1);
          break;
        default:
          break;
      }
    }
    void LevoitSensor::dump_config() { LOG_SENSOR("", "Levoit Sensor", this); }



  } // namespace levoit
} // namespace esphome
