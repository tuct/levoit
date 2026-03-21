#include "levoit_switch.h"
#include "../levoit.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG = "levoit.switch";
    void LevoitSwitch::setup()
    {
    }
    void LevoitSwitch::write_state(bool state)
    {
      // Optimistic update for HA UI
      this->publish_state(state);

      if (!parent_)
      {
        ESP_LOGW(TAG, "No parent set for switch");
        return;
      }

      parent_->on_switch_command(this->type_, state);
    }
    void LevoitSwitch::dump_config() { LOG_SWITCH("", "Levoit Switch", this); }
  } // namespace levoit
} // namespace esphome
