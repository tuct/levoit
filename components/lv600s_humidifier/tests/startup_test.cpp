#include "../lv600s_startup.h"
#include <cassert>

struct Device {
  bool fresh = true, powered = false, ready = false;
  int mode = 3, mist = 9, warm = 0;
  float duration = 120;
  unsigned timers_sent = 0, mist_sent = 0, heat_sent = 0;
  bool has_status() const { return fresh; }
  bool is_power_on() const { return powered; }
  bool can_apply_startup_setting() const { return ready; }
  int get_mode() const { return mode; }
  int get_mist_level() const { return mist; }
  int get_warm_level() const { return warm; }
  float get_timer_duration() const { return duration; }
  bool set_manual_mist_level(int n) { ++mist_sent; mode = 1; mist = n; return true; }
  bool set_warm_level(int n) { ++heat_sent; warm = n; return true; }
  bool set_timer_seconds(unsigned n) { ++timers_sent; duration = n; return true; }
};

int main() {
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
