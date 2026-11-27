// Builds the Levoit protocol core as an ESP-IDF component and uses it the way
// a firmware would: implement LevoitPlatform and LevoitHost, own one
// LevoitDevice, feed it bytes, tick it. The platform here is the smallest
// thing that compiles and runs; a real firmware wires a UART driver, esp_timer
// one-shots and NVS behind the same seven methods (tadhglewis/levoit-matter
// does). Never flashed: CI only builds it (docs/shared-core.md, step 4).
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>

#include <esp_log.h>
#include <esp_timer.h>

#include "levoit_device.h"

static const char *TAG = "levoit-core-check";

struct Platform : levoit::LevoitPlatform {
  std::map<std::string, std::function<void()>> timeouts;
  std::map<std::string, uint32_t> prefs;

  void write(const uint8_t *data, size_t len) override { ESP_LOG_BUFFER_HEX(TAG, data, len); }
  uint32_t millis() override { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }
  void set_timeout(const char *name, uint32_t, std::function<void()> fn) override { timeouts[name] = std::move(fn); }
  void cancel_timeout(const char *name) override { timeouts.erase(name); }
  bool load_u32(const char *key, uint32_t &out) override {
    auto it = prefs.find(key);
    if (it == prefs.end())
      return false;
    out = it->second;
    return true;
  }
  void save_u32(const char *key, uint32_t value) override { prefs[key] = value; }
  bool has_clock() override { return false; }
  bool local_time(uint32_t &) override { return false; }
  levoit::NetworkState network_state() override { return levoit::NetworkState::CONNECTING; }
};

struct Host : levoit::LevoitHost {
  void switch_changed(levoit::SwitchType t, bool v) override { ESP_LOGI(TAG, "switch %d = %d", (int) t, v); }
  void number_changed(levoit::NumberType t, float v) override { ESP_LOGI(TAG, "number %d = %.1f", (int) t, v); }
  void sensor_changed(levoit::SensorType t, float v) override { ESP_LOGI(TAG, "sensor %d = %.1f", (int) t, v); }
  void select_changed(levoit::SelectType t, uint32_t v) override { ESP_LOGI(TAG, "select %d = %u", (int) t, (unsigned) v); }
  void text_changed(levoit::TextSensorType t, const std::string &v) override { ESP_LOGI(TAG, "text %d = %s", (int) t, v.c_str()); }
  void binary_changed(levoit::BinarySensorType t, bool v) override { ESP_LOGI(TAG, "binary %d = %d", (int) t, v); }
  void fan_changed(int power, int speed, int mode) override { ESP_LOGI(TAG, "fan %d %d %d", power, speed, mode); }
};

extern "C" void app_main() {
  static Platform platform;
  static Host host;
  static levoit::LevoitDevice device(&host, &platform);
  device.set_device_model("CORE300S");
  device.begin();

  // "speed 3" from devices/levoit-core300s/uart/status_32_40.txt
  static const uint8_t frame[] = {0xA5, 0x22, 0xD8, 0x17, 0x00, 0x63, 0x01, 0x32, 0x40, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
                                  0x03, 0x00, 0x64, 0x01, 0x03, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  device.receive(frame, sizeof(frame), platform.millis());
  device.tick(platform.millis());

  bool display = false;
  device.try_get_switch_state(levoit::SwitchType::DISPLAY, display);
  ESP_LOGI(TAG, "model %s, fan power %d speed %d mode %d, display %d", device.get_model_name().c_str(),
           device.state().fan.power, device.state().fan.speed, device.state().fan.mode, display);
}
