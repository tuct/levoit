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
      // Optimistic update for HA UI. Switch::publish_state in core
      // ESPHome sets the public `state` field and fires callbacks but
      // does not flip has_state_ — unlike Select::publish_state and
      // Number::publish_state, which both call set_has_state(true).
      // Set it explicitly so `if (entity->has_state())` guards in
      // callers (e.g. command builders that distinguish "user just
      // toggled this" from "never published") behave uniformly across
      // switch / number / select.
      this->publish_state(state);
      this->set_has_state(true);

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
