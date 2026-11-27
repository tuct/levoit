// The device state of components/levoit built without ESPHome, by a plain C++
// compiler (.github/workflows/host-tests.yml). What it proves: levoit_state.h
// depends on nothing but the standard library and types.h, and behaves as the
// hub relies on. See docs/shared-core.md, step 2.
//
//   g++ -std=c++17 -Wall -Wextra tests/host/levoit_state_test.cpp \
//     -o levoit-state-test && ./levoit-state-test
//
// No -I on the component folder, as in an ESPHome build (its VERSION file
// would shadow <version> on a case-insensitive file system).

#include <cmath>
#include <cstdio>
#include <string>

#include "../../components/levoit/levoit_state.h"

using namespace levoit;

static int failures = 0;

#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                        \
    }                                                                    \
  } while (0)

// A slot is unknown until set; set reports a change only on a new value.
static void test_slots() {
  LevoitState s;
  bool b = true;
  CHECK(!s.get_switch(SwitchType::DISPLAY, b));
  CHECK(b);  // untouched when unknown
  CHECK(s.set_switch(SwitchType::DISPLAY, false));
  CHECK(s.get_switch(SwitchType::DISPLAY, b) && !b);
  CHECK(!s.set_switch(SwitchType::DISPLAY, false));  // same value: no change
  CHECK(s.set_switch(SwitchType::DISPLAY, true));

  float f = 0;
  CHECK(s.set_number(NumberType::TIMER, 30));
  CHECK(!s.set_number(NumberType::TIMER, 30));
  CHECK(s.get_number(NumberType::TIMER, f) && f == 30);
  CHECK(s.set_sensor(SensorType::PM25, 12));
  CHECK(s.get_sensor(SensorType::PM25, f) && f == 12);
  // NaN never compares equal, so a NaN sensor publishes each time, as the
  // entity compare did before.
  CHECK(s.set_sensor(SensorType::AQI, NAN));
  CHECK(s.set_sensor(SensorType::AQI, NAN));

  uint32_t u = 0;
  CHECK(s.set_select(SelectType::FAN_OPERATING_MODE_SELECT, 2));
  CHECK(s.get_select(SelectType::FAN_OPERATING_MODE_SELECT, u) && u == 2);

  std::string t;
  CHECK(s.set_text(TextSensorType::MCU_VERSION, "1.0.0"));
  CHECK(!s.set_text(TextSensorType::MCU_VERSION, "1.0.0"));
  CHECK(s.get_text(TextSensorType::MCU_VERSION, t) && t == "1.0.0");

  CHECK(s.set_binary(BinarySensorType::FILTER_LOW, true));
  CHECK(s.get_binary(BinarySensorType::FILTER_LOW, b) && b);
}

// Slots are independent: setting one leaves the others unknown.
static void test_independence() {
  LevoitState s;
  bool b = false;
  CHECK(s.set_switch(SwitchType::CHILD_LOCK, true));
  CHECK(!s.get_switch(SwitchType::DISPLAY, b));
  CHECK(!s.get_switch(SwitchType::MUTE, b));
  float f = 0;
  CHECK(!s.get_number(NumberType::TIMER, f));
}

// An index past the arrays is ignored rather than written out of bounds.
static void test_out_of_range() {
  LevoitState s;
  const auto bad_switch = static_cast<SwitchType>(SWITCH_SLOTS + 3);
  const auto bad_number = static_cast<NumberType>(NUMBER_SLOTS);
  CHECK(!s.set_switch(bad_switch, true));
  bool b = false;
  CHECK(!s.get_switch(bad_switch, b));
  CHECK(!s.set_number(bad_number, 1));
  float f = 0;
  CHECK(!s.get_number(bad_number, f));
}

// The fan merges -1 sentinels: a report without a speed keeps the last one.
static void test_fan() {
  LevoitState s;
  CHECK(s.fan.power == -1 && s.fan.speed == -1 && s.fan.mode == -1);
  CHECK(s.set_fan(1, 3, 0));
  CHECK(s.fan.power == 1 && s.fan.speed == 3 && s.fan.mode == 0);
  CHECK(s.set_fan(-1, -1, 1));  // Sleep: no fan level in the frame
  CHECK(s.fan.power == 1 && s.fan.speed == 3 && s.fan.mode == 1);
  CHECK(!s.set_fan(-1, -1, -1));
  CHECK(!s.set_fan(1, 3, 1));
  CHECK(s.set_fan(0, -1, -1));
  CHECK(s.fan.power == 0 && s.fan.speed == 3);
}

// What the hub does with it: a requested value is read back by a builder
// until the MCU reports, and only a changed report reaches the host.
struct CountingHost : LevoitHost {
  int switches = 0, numbers = 0, sensors = 0, selects = 0, texts = 0, binaries = 0, fans = 0;
  void switch_changed(SwitchType, bool) override { switches++; }
  void number_changed(NumberType, float) override { numbers++; }
  void sensor_changed(SensorType, float) override { sensors++; }
  void select_changed(SelectType, uint32_t) override { selects++; }
  void text_changed(TextSensorType, const std::string &) override { texts++; }
  void binary_changed(BinarySensorType, bool) override { binaries++; }
  void fan_changed(int, int, int) override { fans++; }
};

static void test_host_sees_changes_only() {
  LevoitState s;
  CountingHost host;
  auto publish_number = [&](NumberType t, float v) {
    if (s.set_number(t, v))
      host.number_changed(t, v);
  };
  // on_number_command: requested value, no host call (the entity has it)
  s.set_number(NumberType::TIMER, 45);
  float f = 0;
  CHECK(s.get_number(NumberType::TIMER, f) && f == 45);  // the builder's read
  publish_number(NumberType::TIMER, 45);                 // MCU confirms: nothing new
  CHECK(host.numbers == 0);
  publish_number(NumberType::TIMER, 44);                 // countdown moved
  CHECK(host.numbers == 1);
  publish_number(NumberType::TIMER, 44);
  CHECK(host.numbers == 1);
}

int main() {
  test_slots();
  test_independence();
  test_out_of_range();
  test_fan();
  test_host_sees_changes_only();
  if (failures == 0)
    std::printf("levoit device state: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
