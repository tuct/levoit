#pragma once
// What the protocol core needs from the platform it runs on, and nothing
// more: a UART to write to, a clock, named one-shot timers, two saved
// counters, the local time when there is a clock, and the network state for
// the Wi-Fi LED. The ESPHome component implements it with UARTDevice,
// Component::set_timeout, preferences, the time component and network::; a
// Matter firmware with the IDF driver, esp_timer and NVS. Builds without
// ESPHome (docs/shared-core.md, step 3).
#include <cstddef>
#include <cstdint>
#include <functional>

namespace esphome {
namespace levoit {

enum class NetworkState : uint8_t {
  CONNECTED,   // the Wi-Fi LED goes solid
  CONNECTING,  // not connected and not disabled: the LED blinks
  DISABLED,    // the LED goes off
};

class LevoitPlatform {
 public:
  virtual ~LevoitPlatform() = default;
  // Send a complete frame to the MCU and flush it.
  virtual void write(const uint8_t *data, size_t len) = 0;
  // Milliseconds since boot; wraps every 49.7 days, the core subtracts.
  virtual uint32_t millis() = 0;
  // Run fn once after ms milliseconds. A second call with the same name
  // replaces the first; cancel_timeout drops it. Called from the main loop.
  virtual void set_timeout(const char *name, uint32_t ms, std::function<void()> fn) = 0;
  virtual void cancel_timeout(const char *name) = 0;
  // Persistent counters (used CADR, runtime), keyed by a short name that
  // stays the same across firmware versions.
  virtual bool load_u32(const char *key, uint32_t &out) = 0;
  virtual void save_u32(const char *key, uint32_t value) = 0;
  // Local time as seconds since the epoch, in the local zone, not UTC: that
  // is what the Eleva 300X's MCU expects. False without a clock or before it
  // synced; the platform calls LevoitDevice::on_time_synced once it has.
  virtual bool has_clock() = 0;
  virtual bool local_time(uint32_t &epoch_local) = 0;
  virtual NetworkState network_state() = 0;
};

}  // namespace levoit
}  // namespace esphome
