#include "levoit_select.h"
#include "../levoit.h"
#include "../types.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select_traits.h"

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.select";

    void LevoitSelect::setup() {
      switch (this->type_) {
        case SelectType::AUTO_MODE:
          if (parent_ && parent_->get_model() == ModelType::CORE600S)
            this->traits.set_options({"Default", "Quiet", "Room Size", "ECO"});
          else
            this->traits.set_options({"Default", "Quiet", "Room Size"});
          break;
        case SelectType::SLEEP_MODE:
          this->traits.set_options({"Default","Custom"});
          break;
        case SelectType::QUICK_CLEAN_FAN_LEVEL:
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});
          break;
        case SelectType::WHITE_NOISE_FAN_LEVEL:
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});
          break;
        case SelectType::SLEEP_MODE_FAN_MODE_LEVEL:
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});
          break;
        case SelectType::DAYTIME_FAN_MODE_LEVEL:
          this->traits.set_options({"Auto","Low","Medium","High","Highest","Pet"});
          break;
        case SelectType::NIGHTLIGHT:
          this->traits.set_options({"Off","Mid","Full"});
          break;
        case SelectType::LIGHT_MODE:
          this->traits.set_options({"Off", "Nightlight", "Breathing"});
          break;
        case SelectType::WHITE_NOISE_SOUND:
          this->traits.set_options({
              "Sound 01", "Sound 02", "Sound 03", "Sound 04", "Sound 05",
              "Sound 06", "Sound 07", "Sound 08", "Sound 09", "Sound 10",
              "Sound 11", "Sound 12", "Sound 13", "Sound 14", "Sound 15"});
          break;
        default:
          break;
      }
    }
    void LevoitSelect::dump_config() { LOG_SELECT("", "Levoit Select", this); }

    void LevoitSelect::control(const std::string &value)
    {
      // Find index of the selected option
      const auto &options = this->traits.get_options();
      auto it = std::find(options.begin(), options.end(), value);

      if (it == options.end()) {
        ESP_LOGW(TAG, "Unknown select option: %s", value.c_str());
        return;
      }

      uint32_t index = it - options.begin();

      // Optimistic update for HA UI (string!)
      this->publish_state(value);

      if (!parent_) {
        ESP_LOGW(TAG, "No parent set for select");
        return;
      }

      // Send numeric command to device
      parent_->on_select_command(this->type_, index);
    }

  } // namespace levoit
} // namespace esphome
