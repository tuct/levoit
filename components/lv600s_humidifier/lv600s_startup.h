#pragma once

#include <cstdint>

namespace lv600s_startup {
constexpr int16_t unset = -1;

class Settings {
 public:
  template<class Device>
  void apply(Device &device, int16_t &mist, int16_t &warm, int16_t &minutes) {
    if (!device.has_status()) return;
    if (!device.is_power_on()) {
      mist_sent_ = warm_sent_ = timer_sent_ = unset;
      return;
    }
    if (!device.can_apply_startup_setting()) return;

    const bool manual = device.get_mode() == 1;
    if (mist >= 1 && advance_(
          mist, mist_sent_, manual && device.get_mist_level() == mist,
          [&](int16_t level) { return device.set_manual_mist_level(level); })) return;
    if (warm >= 0 && device.get_mode() != 2 && advance_(
          warm, warm_sent_, device.get_warm_level() == warm,
          [&](int16_t level) { return device.set_warm_level(level); })) return;
    // Require a new, confirmed start even when an earlier countdown used the same duration.
    if (minutes > 0) {
      advance_(minutes, timer_sent_,
               timer_sent_ == minutes && device.get_timer_duration() == minutes * 60,
               [&](int16_t value) { return device.set_timer_seconds(uint32_t(value) * 60); });
    }
  }

 private:
  template<class Send>
  static bool advance_(int16_t &value, int16_t &sent, bool confirmed, Send send) {
    if (confirmed) {
      value = sent = unset;
      return false;
    }
    // The UART queue handles retries. Never restart a timer on every update.
    if (sent == value) return false;
    if (send(value)) sent = value;
    return true;
  }

  int16_t mist_sent_{unset}, warm_sent_{unset}, timer_sent_{unset};
};
}  // namespace lv600s_startup
