#include "../lv600s_startup.h"
#include <cassert>
#include <initializer_list>

struct Device {
  bool fresh = true, powered = false, ready = false;
  bool timer_reported = false, accepts_timer = true;
  int mode = 3, mist = 9, warm = 0;
  float duration = 120, remaining = NAN;
  unsigned last_timer_seconds = 0;
  unsigned timers_sent = 0, mist_sent = 0, heat_sent = 0;
  bool has_status() const { return fresh; }
  bool is_power_on() const { return powered; }
  bool can_apply_startup_setting() const { return ready; }
  int get_mode() const { return mode; }
  int get_mist_level() const { return mist; }
  int get_warm_level() const { return warm; }
  float get_timer_duration() const { return duration; }
  float get_timer_remaining() const { return remaining; }
  bool has_timer_report() const { return timer_reported; }
  bool set_manual_mist_level(int n) { ++mist_sent; mode = 1; mist = n; return true; }
  bool set_warm_level(int n) { ++heat_sent; warm = n; return true; }
  bool set_timer_seconds(unsigned n) {
    ++timers_sent;
    last_timer_seconds = n;
    if (!accepts_timer) return false;
    duration = n;
    return true;
  }
};

void verify_timer_display() {
  using namespace lv600s_startup;
  Device device;
  device.duration = NAN;
  assert(timer_duration(device, unset) == 0);
  assert(timer_remaining(device) == 0 && timer_minutes(device, unset) == 0);
  assert(timer_duration(device, 2) == 120 && timer_minutes(device, 2) == 2);

  device.powered = true;
  assert(std::isnan(timer_duration(device, 2)) && std::isnan(timer_remaining(device)));
  assert(std::isnan(timer_minutes(device, 2)));
  device.timer_reported = true;
  device.duration = 120;
  device.remaining = 61;
  assert(timer_duration(device, unset) == 120 && timer_minutes(device, unset) == 2);
  device.duration = device.remaining = NAN;
  assert(std::isnan(timer_duration(device, unset)) && std::isnan(timer_minutes(device, unset)));
  device.duration = device.remaining = 0;
  assert(timer_remaining(device) == 0 && timer_minutes(device, unset) == 0);
  device.fresh = false;
  assert(std::isnan(timer_remaining(device)) && std::isnan(timer_minutes(device, 2)));
}

void verify_timer_cancellation() {
  using namespace lv600s_startup;
  for (bool fresh : {false, true}) {
    Device device;
    device.fresh = fresh;
    device.powered = true;
    device.accepts_timer = false;
    int16_t timer = 2;
    set_timer_minutes(device, timer, 0);
    assert(timer == unset);
    assert(device.timers_sent == unsigned(fresh) && device.last_timer_seconds == 0);
    device.fresh = device.ready = true;
    device.powered = false;
    Settings settings;
    int16_t mist = unset, warm = unset;
    settings.apply(device, mist, warm, timer);
    device.powered = true;
    settings.apply(device, mist, warm, timer);
    assert(device.timers_sent == unsigned(fresh));
  }

  Device standby;
  int16_t timer = unset;
  set_timer_minutes(standby, timer, 2);
  assert(timer == 2 && standby.timers_sent == 0);
  standby.remaining = 61;
  set_timer_minutes(standby, timer, 0);
  assert(timer == unset && standby.timers_sent == 0);
}

int main() {
  verify_timer_display();
  verify_timer_cancellation();
  using namespace lv600s_startup;
  Device device;
  Settings settings;
  int16_t mist = 5, warm = 2, timer = 2;
  const auto apply = [&] { settings.apply(device, mist, warm, timer); };
  apply();
  device.powered = true;
  apply();
  assert(device.mist_sent == 0 && device.heat_sent == 0 && device.timers_sent == 0);
  device.ready = true;
  apply();
  assert(device.mode == 1 && device.mist == 5 && device.mist_sent == 1 && mist == 5);
  apply();
  assert(mist == unset && device.warm == 2 && device.heat_sent == 1 && warm == 2);
  apply();
  assert(warm == unset && device.timers_sent == 1 && timer == 2);
  apply();
  assert(timer == unset);
  device.powered = false;
  apply();
  device.powered = true;
  apply();
  assert(device.timers_sent == 1);

  device.mode = 2;
  warm = 3;
  apply();
  assert(device.heat_sent == 1 && warm == 3);
  device.mode = 1;
  apply();
  assert(device.heat_sent == 2 && device.warm == 3);
}
