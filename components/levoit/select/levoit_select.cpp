#include "levoit_select.h"
#include "../levoit.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select_traits.h"

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.select";

    void LevoitSelect::setup() {
      // pick default
      this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);

      switch (this->type_) {
        case SelectType::AUTO_MODE: 

          this->traits.set_options({"Default","Quiet","Room Size"});
          break;
        case SelectType::SLEEP_MODE:
          this->traits.set_options({"Default","Custom"}); 
          break;
        case SelectType::QUICK_CLEAN_FAN_LEVEL:
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"}); 
          break;
        case SelectType::WHITE_NOISE_FAN_LEVEL:
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});  
          break;
        case SelectType::SLEEP_MODE_FAN_MODE_LEVEL:
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});    
          break;
        case SelectType::DAYTIME_FAN_MODE_LEVEL:
          this->set_entity_category(EntityCategory::ENTITY_CATEGORY_CONFIG);
          this->traits.set_options({"Auto","Low","Medium","High","Highest","Pet"});
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
