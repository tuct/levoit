#pragma once
// The device state the protocol core owns, and the interface it reports
// changes through. Builds without ESPHome (docs/shared-core.md, step 2).
//
// The decoders (*_status.cpp) describe what the MCU reports in the typed
// enums of types.h: publish_switch(SwitchType::DISPLAY, true) and so on. The
// command builders (*_commands.cpp) read the same slots back. LevoitState is
// the home of those slots: one known-flag and value per enum entry, plus the
// fan's power/speed/mode. A setter returns whether the value changed, and the
// hub calls the LevoitHost method for that slot only then. The ESPHome host
// forwards it to the matching entity; a Matter host maps it to a cluster.
//
// A slot holds the last reported value, or the last requested one when a
// command was sent and the MCU has not answered yet, the same way an ESPHome
// entity publishes optimistically before the report comes back.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "types.h"

namespace levoit {

// Enough slots for every entry of the enums in types.h, with headroom. An
// index past the end is ignored by the setters and reads as unknown.
constexpr size_t SWITCH_SLOTS = 16;
constexpr size_t NUMBER_SLOTS = 24;
constexpr size_t SENSOR_SLOTS = 16;
constexpr size_t SELECT_SLOTS = 16;
constexpr size_t TEXT_SLOTS = 16;
constexpr size_t BINARY_SLOTS = 8;
static_assert(static_cast<size_t>(SwitchType::DYNAMIC_SLEEP) < SWITCH_SLOTS, "SwitchType outgrew SWITCH_SLOTS");
static_assert(static_cast<size_t>(NumberType::QUIET_END) < NUMBER_SLOTS, "NumberType outgrew NUMBER_SLOTS");
static_assert(static_cast<size_t>(SensorType::AMBIENT_LIGHT) < SENSOR_SLOTS, "SensorType outgrew SENSOR_SLOTS");
static_assert(static_cast<size_t>(SelectType::QUIET_TRIGGER) < SELECT_SLOTS, "SelectType outgrew SELECT_SLOTS");
static_assert(static_cast<size_t>(TextSensorType::MODEL) < TEXT_SLOTS, "TextSensorType outgrew TEXT_SLOTS");
static_assert(static_cast<size_t>(BinarySensorType::POWER_BUTTON_HELD) < BINARY_SLOTS, "BinarySensorType outgrew BINARY_SLOTS");

template<typename T> struct StateSlot {
  T value{};
  bool known{false};
  // Stores v; true when the slot was unknown or held a different value.
  bool set(const T &v) {
    const bool changed = !known || !(value == v);
    value = v;
    known = true;
    return changed;
  }
  // Copies the value into out when known, and says so.
  bool get(T &out) const {
    if (!known)
      return false;
    out = value;
    return true;
  }
};

// power / speed / mode as the decoders report them: -1 means "not in this
// frame, keep what we had" (see Levoit::apply_fan_status). A field is known
// once any frame carried it.
struct FanState {
  int power{-1};
  int speed{-1};
  int mode{-1};
  // The manual speed the user last picked, where the MCU reports it apart
  // from the running speed (Core 300S 01 32 40: [5], kept in Auto, Sleep and
  // while off, when speed is the running speed [9]); -1 when not reported.
  int selected_speed{-1};
  // Merges a report; true when a carried field changed.
  bool set(int p, int s, int m) {
    bool changed = false;
    if (p != -1 && p != power) { power = p; changed = true; }
    if (s != -1 && s != speed) { speed = s; changed = true; }
    if (m != -1 && m != mode) { mode = m; changed = true; }
    return changed;
  }
  // The manual speed the fan returns to: selected_speed where the status
  // carries it apart from the running speed, otherwise speed, which already
  // holds the manual value on the layouts that do not separate the two.
  int manual_speed() const { return selected_speed != -1 ? selected_speed : speed; }
};

struct LevoitState {
  std::array<StateSlot<bool>, SWITCH_SLOTS> switches;
  std::array<StateSlot<float>, NUMBER_SLOTS> numbers;
  std::array<StateSlot<float>, SENSOR_SLOTS> sensors;
  std::array<StateSlot<uint32_t>, SELECT_SLOTS> selects;
  std::array<StateSlot<std::string>, TEXT_SLOTS> texts;
  std::array<StateSlot<bool>, BINARY_SLOTS> binaries;
  FanState fan;

  bool set_switch(SwitchType t, bool v) { return set_(switches, t, v); }
  bool set_number(NumberType t, float v) { return set_(numbers, t, v); }
  bool set_sensor(SensorType t, float v) { return set_(sensors, t, v); }
  bool set_select(SelectType t, uint32_t v) { return set_(selects, t, v); }
  bool set_text(TextSensorType t, const std::string &v) { return set_(texts, t, v); }
  bool set_binary(BinarySensorType t, bool v) { return set_(binaries, t, v); }
  bool set_fan(int power, int speed, int mode) { return fan.set(power, speed, mode); }

  bool get_switch(SwitchType t, bool &out) const { return get_(switches, t, out); }
  bool get_number(NumberType t, float &out) const { return get_(numbers, t, out); }
  bool get_sensor(SensorType t, float &out) const { return get_(sensors, t, out); }
  bool get_select(SelectType t, uint32_t &out) const { return get_(selects, t, out); }
  bool get_text(TextSensorType t, std::string &out) const { return get_(texts, t, out); }
  bool get_binary(BinarySensorType t, bool &out) const { return get_(binaries, t, out); }

 private:
  template<typename Slots, typename E, typename V> static bool set_(Slots &slots, E type, const V &v) {
    const auto i = static_cast<size_t>(type);
    return i < slots.size() && slots[i].set(v);
  }
  template<typename Slots, typename E, typename V> static bool get_(const Slots &slots, E type, V &out) {
    const auto i = static_cast<size_t>(type);
    return i < slots.size() && slots[i].get(out);
  }
};

// What a host hears from the core. Each method is called when that slot
// changed, with the new value; fan_changed on every fan status report, with
// -1 for the fields the report did not carry. A host that needs the manual
// speed the fan will resume to reads state().fan.manual_speed() inside
// fan_changed, which fires on every report.
class LevoitHost {
 public:
  virtual ~LevoitHost() = default;
  virtual void switch_changed(SwitchType type, bool state) = 0;
  virtual void number_changed(NumberType type, float value) = 0;
  virtual void sensor_changed(SensorType type, float value) = 0;
  virtual void select_changed(SelectType type, uint32_t value) = 0;
  virtual void text_changed(TextSensorType type, const std::string &value) = 0;
  virtual void binary_changed(BinarySensorType type, bool state) = 0;
  virtual void fan_changed(int power, int speed, int mode) = 0;
  // Sprout LED ring, as the MCU reports it. Not part of the typed slots yet;
  // a host without a Sprout mapping can leave it.
  virtual void sprout_light_changed(bool on, float brightness, float color_temp, bool breathing) {
    (void) on;
    (void) brightness;
    (void) color_temp;
    (void) breathing;
  }
};

}  // namespace levoit
