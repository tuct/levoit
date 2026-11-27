// The whole protocol core of components/levoit built without ESPHome, by a
// plain C++ compiler (.github/workflows/host-tests.yml): LevoitDevice, the
// frame parser, the decoders and builders of every model, driven through a
// fake platform and a fake host. What it proves: the core depends on nothing
// but the standard library, and captured frames decode to the state the host
// hears about. See docs/shared-core.md, step 3.
//
// It also carries the parser and ACK cases that lived in
// components/levoit/tests/test_uart_integrity.py, which compiled the parser
// by slicing levoit.cpp as text.
//
// No -I on the component folder, as in an ESPHome build (its VERSION file
// would shadow <version> on a case-insensitive file system).

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../../components/levoit/levoit_device.h"
#include "../../components/levoit/levoit_message.h"

using namespace levoit;

static int failures = 0;

#define CHECK(cond)                                                      \
  do {                                                                   \
    if (!(cond)) {                                                       \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                                        \
    }                                                                    \
  } while (0)

using Bytes = std::vector<uint8_t>;

// "A5 22 D8 17 ..." as bytes
static Bytes hex(const char *text) {
  Bytes out;
  std::istringstream in(text);
  std::string tok;
  while (in >> tok)
    out.push_back(static_cast<uint8_t>(std::strtoul(tok.c_str(), nullptr, 16)));
  return out;
}

// A well-formed MCU push (0x22) with a payload of the given size, the
// Core 600S status type, as test_uart_integrity.py built it.
static Bytes frame(size_t payload) {
  Bytes f(10 + payload, 0);
  f[0] = 0xA5;
  f[1] = 0x22;
  f[3] = (payload + 4) & 255;
  f[4] = (payload + 4) >> 8;
  f[6] = 1;
  f[7] = 0x40;
  f[8] = 0x41;
  levoit_finalize_message(f, 7);
  return f;
}

struct FakePlatform : LevoitPlatform {
  std::vector<Bytes> tx;
  uint32_t now{0};
  std::map<std::string, std::function<void()>> timeouts;
  std::map<std::string, uint32_t> prefs;
  bool clock{false};
  uint32_t local{0};
  NetworkState network{NetworkState::CONNECTING};

  void write(const uint8_t *data, size_t len) override { tx.emplace_back(data, data + len); }
  uint32_t millis() override { return now; }
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
  bool has_clock() override { return clock; }
  bool local_time(uint32_t &epoch_local) override {
    if (!clock || local == 0)
      return false;
    epoch_local = local;
    return true;
  }
  NetworkState network_state() override { return network; }

  // Fire a named timeout once, as the platform would.
  bool fire(const char *name) {
    auto it = timeouts.find(name);
    if (it == timeouts.end())
      return false;
    auto fn = std::move(it->second);
    timeouts.erase(it);
    fn();
    return true;
  }
  // The ACKs the core sent: one per accepted MCU push, so a frame count.
  size_t acks() const {
    size_t n = 0;
    for (const auto &f : tx)
      if (f.size() >= 2 && (f[1] == 0x12 || f[1] == 0x52))
        n++;
    return n;
  }
  // The last frame carrying this command type, if any.
  const Bytes *last_with(uint8_t ptype0, uint8_t ptype1) const {
    for (auto it = tx.rbegin(); it != tx.rend(); ++it)
      if (it->size() >= 9 && (*it)[7] == ptype0 && (*it)[8] == ptype1)
        return &*it;
    return nullptr;
  }
};

struct FakeHost : LevoitHost {
  std::map<SwitchType, bool> switches;
  std::map<NumberType, float> numbers;
  std::map<SensorType, float> sensors;
  std::map<SelectType, uint32_t> selects;
  std::map<TextSensorType, std::string> texts;
  std::map<BinarySensorType, bool> binaries;
  int fan_power{-1}, fan_speed{-1}, fan_mode{-1}, fan_reports{0};

  void switch_changed(SwitchType t, bool v) override { switches[t] = v; }
  void number_changed(NumberType t, float v) override { numbers[t] = v; }
  void sensor_changed(SensorType t, float v) override { sensors[t] = v; }
  void select_changed(SelectType t, uint32_t v) override { selects[t] = v; }
  void text_changed(TextSensorType t, const std::string &v) override { texts[t] = v; }
  void binary_changed(BinarySensorType t, bool v) override { binaries[t] = v; }
  void fan_changed(int p, int s, int m) override {
    fan_reports++;
    if (p != -1) fan_power = p;
    if (s != -1) fan_speed = s;
    if (m != -1) fan_mode = m;
  }
};

struct Rig {
  FakePlatform platform;
  FakeHost host;
  LevoitDevice device{&host, &platform};
  explicit Rig(const char *model) { device.set_device_model(model); }
  void feed(const Bytes &bytes, uint32_t now) {
    platform.now = now;
    device.receive(bytes.data(), bytes.size(), now);
  }
};

// Core 300S on the 19-byte 01 32 40 status, frames from
// devices/levoit-core300s/uart/status_32_40.txt: what the host hears, and the
// ACK the core answers with, byte for byte as the capture shows it.
static void test_core300s_capture() {
  Rig rig("CORE300S");
  messageUpCounter = 0x42;  // the ESP's counter when the capture's ack was sent

  // "speed 3" at 216.110
  rig.feed(hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00"), 1000);
  CHECK(rig.host.fan_power == 1 && rig.host.fan_speed == 3 && rig.host.fan_mode == 0);
  CHECK(rig.device.state().fan.selected_speed == 3);
  CHECK(rig.host.switches[SwitchType::DISPLAY] == true);
  CHECK(rig.host.switches[SwitchType::CHILD_LOCK] == false);
  CHECK(rig.host.sensors[SensorType::AQI] == 1);
  CHECK(rig.host.sensors[SensorType::PM25] == 5);
  CHECK(rig.host.texts[TextSensorType::MCU_VERSION] == "1.0.0");
  CHECK(rig.platform.tx.size() == 1);
  CHECK(rig.platform.tx.back() == hex("A5 12 42 04 FF 90 01 32 40 00"));
  CHECK(messageUpCounter == 0x43);

  // "sleep. Mode 01, running speed 00" at 232.370: speed 0, display dark
  rig.feed(hex("A5 22 DC 17 00 61 01 32 40 00 00 00 01 01 01 03 00 64 01 00 00 01 05 00 00 00 00 00 00"), 2000);
  CHECK(rig.host.fan_power == 1 && rig.host.fan_speed == 0 && rig.host.fan_mode == 1);
  CHECK(rig.device.state().fan.selected_speed == 3);
  CHECK(rig.device.state().fan.manual_speed() == 3);  // running speed is 0 in Sleep
  CHECK(rig.host.switches[SwitchType::DISPLAY] == false);

  // "power off, at the panel" at 189.370: off, the kept manual speed 1
  rig.feed(hex("A5 22 D4 17 00 D6 01 32 40 00 00 00 01 00 00 01 00 00 01 FF 00 01 01 00 00 00 00 00 00"), 3000);
  CHECK(rig.host.fan_power == 0 && rig.host.fan_speed == 1 && rig.host.fan_mode == 0);
  CHECK(rig.device.state().fan.selected_speed == 1);
  CHECK(rig.host.fan_reports == 3);
  CHECK(rig.platform.acks() == 3);

  // The state the builders read is what the host heard
  bool display = true;
  CHECK(rig.device.try_get_switch_state(SwitchType::DISPLAY, display) && !display);
  CHECK(rig.device.state().fan.power == 0 && rig.device.state().fan.speed == 1);
}

// A command from the host: the requested value is in the state at once, the
// frame goes out well formed.
static void test_command() {
  Rig rig("CORE300S");
  rig.feed(hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00"), 1000);
  const size_t sent = rig.platform.tx.size();
  rig.device.on_switch_command(SwitchType::DISPLAY, false);
  bool display = true;
  CHECK(rig.device.try_get_switch_state(SwitchType::DISPLAY, display) && !display);
  CHECK(rig.platform.tx.size() == sent + 1);
  const Bytes &f = rig.platform.tx.back();
  CHECK(f.size() >= 10 && f[0] == 0xA5 && f[1] == 0x22);
  CHECK(levoit_checksum(f.data(), f.size()) == f[5]);
  CHECK(f[3] == f.size() - 6);

  rig.device.on_fan_command(-1, 2, 0);
  CHECK(rig.device.state().fan.selected_speed == 2);
  Rig vital("VITAL100S");
  vital.device.on_fan_command(-1, 2, 0);
  CHECK(vital.device.state().fan.selected_speed == -1);
}

// The dedup gate: an unchanged push is skipped, every changed one is decoded.
// AQI 1 -> 2 with PM2.5 5 -> 3 left the old 8-bit fingerprint unchanged.
static void test_dedup() {
  Rig rig("CORE300S");
  Bytes push = hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00");
  rig.feed(push, 1000);
  rig.feed(push, 2000);
  CHECK(rig.host.fan_reports == 1);

  push[21] = 0x02;
  push[22] = 0x03;
  push[5] = levoit_checksum(push.data(), push.size());
  rig.feed(push, 3000);
  CHECK(rig.host.fan_reports == 2);
  CHECK(rig.host.sensors[SensorType::AQI] == 2);
  CHECK(rig.host.sensors[SensorType::PM25] == 3);
}

// A command the MCU does not carry out: the pushes after it repeat the last
// payload. Within the grace window they stay skipped (a fast pusher still
// sends the old state while the MCU applies a command); the first one after
// it is decoded once, so the requested value does not stay in the state.
static void test_refused_command() {
  Rig rig("CORE300S");
  const Bytes push = hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00");
  rig.feed(push, 1000);
  CHECK(rig.host.switches[SwitchType::DISPLAY] == true);

  rig.device.on_switch_command(SwitchType::DISPLAY, false);
  rig.host.switches.clear();
  rig.feed(push, 1000 + LevoitDevice::COMMAND_GRACE_MS - 100);
  bool display = true;
  CHECK(rig.device.try_get_switch_state(SwitchType::DISPLAY, display) && !display);
  CHECK(rig.host.switches.empty());
  CHECK(rig.host.fan_reports == 1);

  rig.feed(push, 1000 + LevoitDevice::COMMAND_GRACE_MS + 100);
  CHECK(rig.device.try_get_switch_state(SwitchType::DISPLAY, display) && display);
  CHECK(rig.host.switches.count(SwitchType::DISPLAY) == 1 && rig.host.switches[SwitchType::DISPLAY] == true);
  CHECK(rig.host.fan_reports == 2);

  rig.feed(push, 1000 + LevoitDevice::COMMAND_GRACE_MS + 900);
  CHECK(rig.host.fan_reports == 2);
}

// begin(): counters restored through the platform, the Core 300S asks for its
// status once if nothing arrived, CADR tracking runs on the device state.
static void test_capabilities() {
  struct { const char *model; uint8_t speeds; bool automode; bool aq; const char *disp; } cases[] = {
      {"CORE200S", 3, false, false, "Core 200S"},
      {"CORE300S", 3, true, true, "Core 300S"},
      {"CORE400S", 4, true, true, "Core 400S"},
      {"CORE600S", 4, true, true, "Core 600S"},
      {"SPROUT", 4, true, true, "Sprout"},
      {"EVERESTAIR", 3, true, true, "EverestAir"},
      {"VITAL100S", 4, true, true, "Vital 100S"},
      {"VITAL200S", 4, true, true, "Vital 200S"},
      {"ELEVA300X", 4, true, true, "Eleva 300X"},
      {"SUPERIOR6000S", 9, true, false, "Superior 6000S"},
  };
  for (auto &c : cases) {
    Rig rig(c.model);
    CHECK(rig.device.speed_count() == c.speeds);
    CHECK(rig.device.has_auto() == c.automode);
    CHECK(rig.device.has_air_quality() == c.aq);
    CHECK(std::string(rig.device.display_name()) == c.disp);
  }
}

static void test_set_filter_percent() {
  for (uint8_t pct = 0; pct <= 100; pct += 10) {
    Rig rig("CORE300S");
    rig.device.note_number_state(NumberType::FILTER_LIFETIME_MONTHS, 6);
    rig.device.begin();
    rig.device.set_filter_percent(pct);
    const int got = (int)(rig.device.calculate_filter_life_left_percent() + 0.5f);
    CHECK(got == pct);
  }
}

static void test_begin_and_tracking() {
  Rig rig("CORE300S");
  rig.platform.prefs[LevoitDevice::PREF_USED_CADR] = 1234;
  rig.platform.prefs[LevoitDevice::PREF_TOTAL_RUNTIME] = 56;
  rig.device.begin();
  CHECK(rig.device.get_used_cadr() == 1234 && rig.device.get_total_runtime() == 56);
  CHECK(rig.platform.last_with(0xE4, 0xA5) == nullptr);  // no Core 200S filter reset
  CHECK(rig.platform.timeouts.count("core300s_status_req1") == 1);
  const size_t before = rig.platform.tx.size();
  CHECK(rig.platform.fire("core300s_status_req1"));
  CHECK(rig.platform.tx.size() == before + 1);
  CHECK(rig.platform.last_with(0x33, 0x40) != nullptr);  // requestCore300sStatus
  // Once a status arrived, the next try asks nothing
  rig.feed(hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00"), 1000);
  const size_t after = rig.platform.tx.size();
  CHECK(rig.platform.fire("core300s_status_req2"));
  CHECK(rig.platform.tx.size() == after);

  // Core 300S: 214 m³/h at speed 3 of 3, full CADR; the minute tick counts
  // runtime and publishes the sensors, but does not save on its own
  CHECK(rig.device.calculate_current_cadr_per_hour() == 214);
  rig.device.track_cadr_usage();
  CHECK(rig.device.get_total_runtime() == 57);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_TOTAL_RUNTIME] == 56);
  CHECK(rig.host.sensors.count(SensorType::FILTER_LIFE_LEFT) == 1);
}

// Filter wear in speed-weighted seconds (#162): migrated once from the old
// cubic-metre counter, accrued per tick, saved per completed hour and at
// shutdown, reset at once.
static void test_filter_accounting() {
  Rig rig("CORE300S");
  rig.platform.prefs[LevoitDevice::PREF_USED_CADR] = 1234;  // old key only
  rig.device.note_number_state(NumberType::FILTER_LIFETIME_MONTHS, 12);
  rig.device.begin();
  // 1234 m³ at 214 m³/h = 20759 s at full CADR, seeded and saved under the new key
  CHECK(rig.device.get_filter_seconds() == 20759);
  CHECK(rig.device.get_used_cadr() == 1234);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 20759);

  // speed 3 of 3: ten seconds of wall time are ten seconds of wear
  rig.feed(hex("A5 22 D8 17 00 63 01 32 40 00 00 00 01 01 00 03 00 64 01 03 00 01 05 00 00 00 00 00 00"), 1000);
  rig.device.tick(1000);
  rig.device.tick(11000);
  CHECK(rig.device.get_filter_seconds() == 20769);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 20759);  // no hour completed: no save

  // Sleep at running speed 0 flags the sleep speed; speed 1 of 3 derated to
  // 63 %: 214 / 3 * 0.63 = 44 m³/h, so ten seconds are two seconds of wear
  rig.feed(hex("A5 22 DC 17 00 61 01 32 40 00 00 00 01 01 01 03 00 64 01 00 00 01 05 00 00 00 00 00 00"), 12000);
  rig.device.state();  // (speed 0 in Sleep: calculate_current_cadr_per_hour is 0)
  rig.device.tick(12000);
  rig.device.tick(22000);
  CHECK(rig.device.get_filter_seconds() == 20769);
  rig.device.on_fan_command(-1, 1, 0);  // Manual, speed 1
  rig.device.tick(23000);
  rig.device.tick(33000);
  CHECK(rig.device.calculate_current_cadr_per_hour() == 71);
  CHECK(rig.device.get_filter_seconds() == 20769 + 3);  // 10 s * 71 / 214 = 3.3

  // off: nothing accrues
  rig.device.on_fan_command(0, -1, -1);
  rig.device.tick(34000);
  rig.device.tick(94000);
  CHECK(rig.device.get_filter_seconds() == 20772);

  // back to full speed at 94 s until the hour boundary (21600 s): the wear
  // accrues from the command on, and is saved at the boundary, once
  rig.device.on_fan_command(1, 3, 0);
  rig.device.tick(94000 + (21600 - 20772) * 1000 - 1000);
  CHECK(rig.device.get_filter_seconds() == 21599);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 20759);
  rig.device.tick(94000 + (21600 - 20772) * 1000);
  CHECK(rig.device.get_filter_seconds() == 21600);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 21600);

  // the percentage is seconds over the lifetime in seconds
  const float left = rig.device.calculate_filter_life_left_percent();
  CHECK(left > 99.9f && left < 100.0f);  // 21600 of 31,104,000 s

  // shutdown saves whatever is there; a reset saves at once
  rig.device.tick(94000 + (21600 - 20772) * 1000 + 5000);
  CHECK(rig.device.get_filter_seconds() == 21605);
  rig.device.shutdown();
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 21605);
  rig.device.set_used_cadr(0);
  CHECK(rig.device.get_filter_seconds() == 0);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_FILTER_SECONDS] == 0);

  // a fresh device with neither key starts at 0, and set_filter_percent seeds seconds
  Rig fresh("CORE300S");
  fresh.device.note_number_state(NumberType::FILTER_LIFETIME_MONTHS, 12);
  fresh.device.begin();
  CHECK(fresh.device.get_filter_seconds() == 0 && fresh.device.get_used_cadr() == 0);
  fresh.device.set_filter_percent(50);
  CHECK(fresh.device.get_filter_seconds() == 31104000u / 2);
  const float half = fresh.device.calculate_filter_life_left_percent();
  CHECK(half > 49.9f && half < 50.1f);
}

// tick(): the Wi-Fi LED follows the network through the platform.
static void test_wifi_led() {
  Rig rig("VITAL100S");
  rig.platform.network = NetworkState::CONNECTING;
  rig.platform.now = 1000;
  rig.device.tick(1000);
  CHECK(rig.platform.last_with(0x29, 0xA1) != nullptr || rig.platform.tx.size() >= 1);  // blinking sent
  const size_t sent = rig.platform.tx.size();
  rig.platform.network = NetworkState::CONNECTED;
  rig.device.tick(2000);
  CHECK(rig.platform.tx.size() == sent + 1);  // LED off first
  CHECK(rig.platform.timeouts.count("wifi_led_on") == 1);
  CHECK(rig.platform.fire("wifi_led_on"));
  CHECK(rig.platform.tx.size() == sent + 2);  // then solid
}

// Core models: the filter LED starts off, never blinks, is lit while the
// filter is under 5 %, and goes out when it is reset. The Core 200S keeps it off.
static void test_filter_led() {
  auto leds = [](const FakePlatform &p) {
    std::vector<uint8_t> out;
    for (const Bytes &f : p.tx)
      if (f.size() >= 11 && f[7] == 0xE2 && f[8] == 0xA5)
        out.push_back(f.back());
    return out;
  };
  const uint32_t worn = 214u * 24 * 30 * 6 / 100 * 97;  // Core 300S, 6 months: 3 % left
  Rig rig("CORE300S");
  rig.platform.prefs[LevoitDevice::PREF_USED_CADR] = worn;
  rig.device.note_number_state(NumberType::FILTER_LIFETIME_MONTHS, 6);
  rig.device.begin();
  rig.platform.network = NetworkState::CONNECTING;
  rig.device.tick(1000);
  rig.device.tick(2000);
  CHECK(leds(rig.platform) == std::vector<uint8_t>({0x00}));

  rig.device.publish_filter_stats_now();
  rig.device.publish_filter_stats_now();
  rig.platform.network = NetworkState::CONNECTED;
  rig.device.tick(3000);
  CHECK(leds(rig.platform) == std::vector<uint8_t>({0x00, 0x01}));

  rig.device.set_used_cadr(0);
  rig.device.publish_filter_stats_now();
  CHECK(leds(rig.platform) == std::vector<uint8_t>({0x00, 0x01, 0x00}));

  Rig c200("CORE200S");
  c200.platform.prefs[LevoitDevice::PREF_USED_CADR] = 167u * 24 * 30 * 6;
  c200.device.note_number_state(NumberType::FILTER_LIFETIME_MONTHS, 6);
  c200.device.begin();
  c200.device.publish_filter_stats_now();
  CHECK(leds(c200.platform) == std::vector<uint8_t>({0x00}));
}

// The ACK bytes, as test_uart_integrity.py checked them against captures.
static void test_acks() {
  // The stock Core trace replies to sequence 0 with exactly these ten bytes.
  Rig core("CORE600S");
  messageUpCounter = 93;
  core.device.ackMessage(0x40, 0x41, 0);
  CHECK(core.platform.tx.back() == hex("A5 52 00 04 00 81 01 40 41 01"));
  CHECK(messageUpCounter == 93);

  Rig vital("VITAL100S");
  vital.device.ackMessage(0, 0x55, 0xF2);
  const Bytes &v = vital.platform.tx.back();
  CHECK(v[2] == 0xF2 && v[4] == 0 && v[6] == 2);
  CHECK(levoit_checksum(v.data(), v.size()) == v[5]);
  CHECK(messageUpCounter == 93);

  // Match the existing Core 300S ACK convention captured in status_32_40.txt.
  Rig core300("CORE300S");
  messageUpCounter = 0x1C;
  core300.device.ackMessage(0x32, 0x40, 0xD0);
  CHECK(core300.platform.tx.back() == hex("A5 12 1C 04 FF B6 01 32 40 00"));
  CHECK(messageUpCounter == 0x1D);
  messageUpCounter = 255;
  core300.device.ackMessage(0x32, 0x40, 0xD1);
  CHECK(messageUpCounter == 16);
}

// process_message: the protocol version and result byte gate a frame.
static void test_version_guard() {
  Rig guard("CORE600S");
  Bytes status = frame(23);
  guard.device.process_message(status.data(), status.size());
  CHECK(guard.platform.acks() == 1);
  status[6] = 2;
  guard.device.process_message(status.data(), status.size());
  CHECK(guard.platform.acks() == 1);
  status[6] = 1;
  status[9] = 1;
  guard.device.process_message(status.data(), status.size());
  CHECK(guard.platform.acks() == 1);

  Rig vital("VITAL100S");
  status[9] = 0;
  status[6] = 1;
  vital.device.process_message(status.data(), status.size());
  CHECK(vital.platform.acks() == 0);
  status[6] = 2;
  vital.device.process_message(status.data(), status.size());
  CHECK(vital.platform.acks() == 1);
}

// The parser: fragments, bad lengths, bad checksums, silence, bursts, wrap.
static void test_parser() {
  const Bytes good = frame(23);
  Rig d("CORE600S");
  d.feed(good, 100);
  CHECK(d.platform.acks() == 1);

  // Correct 16-bit length, split at a byte boundary between bursts.
  const Bytes long_frame = frame(300);
  d.feed(Bytes(long_frame.begin(), long_frame.begin() + 200), 100);
  CHECK(d.platform.acks() == 1);
  d.feed(Bytes(long_frame.begin() + 200, long_frame.end()), 200);
  CHECK(d.platform.acks() == 2);

  // A partial frame survives a short gap and dies after 250 ms of silence.
  const Bytes header(good.begin(), good.begin() + 6);
  const Bytes tail(good.begin() + 6, good.end());
  Rig gap("CORE600S");
  gap.feed(header, 0);
  gap.feed(tail, 240);
  CHECK(gap.platform.acks() == 1);
  Rig expired("CORE600S");
  expired.feed(header, 0);
  expired.feed(tail, 260);
  CHECK(expired.platform.acks() == 0);
  expired.feed(good, 261);
  CHECK(expired.platform.acks() == 1);

  // A stuck length is discarded before the next burst after 250 ms silence.
  d.feed(hex("A5 22 00 F0 00 00"), 300);
  d.feed(good, 551);
  CHECK(d.platform.acks() == 3);

  // Large high byte and impossible short lengths must resynchronize immediately.
  for (Bytes junk : {hex("A5 22 00 04 FF 00"), hex("A5 22 00 00 00 00")}) {
    junk.insert(junk.end(), good.begin(), good.end());
    const size_t old = d.platform.acks();
    d.feed(junk, 600);
    CHECK(d.platform.acks() == old + 1);
  }
  Bytes corrupt = good;
  corrupt[5] ^= 1;
  corrupt.insert(corrupt.end(), good.begin(), good.end());
  const size_t old = d.platform.acks();
  d.feed(corrupt, 600);
  CHECK(d.platform.acks() == old + 1);

  // A driver may hand over more than the 512-byte buffer in one burst.
  Rig burst("CORE600S");
  Bytes bytes;
  for (unsigned i = 0; i < 30; ++i)
    bytes.insert(bytes.end(), good.begin(), good.end());
  burst.feed(bytes, 100);
  CHECK(burst.platform.acks() == 30);

  // Milliseconds wrap between a partial frame and the next burst.
  Rig wrap("CORE600S");
  wrap.feed(hex("A5 22"), 0xFFFFFFF0);
  wrap.feed(good, 300);
  CHECK(wrap.platform.acks() == 1);
}

// Synthetic fixtures based on the MCU 1.0.9 observations in the local
// devices/levoit-superior-6000s/observations-1.0.9.txt.
// These are NOT captured UART frames.
static void test_superior_power_and_commands() {
  Rig rig("SUPERIOR6000S");
  auto report = [&rig](Bytes payload, uint32_t now) {
    Bytes f(10 + payload.size(), 0);
    f[0] = 0xA5; f[1] = 0x22;
    f[3] = static_cast<uint8_t>(payload.size() + 4);
    f[6] = 2; f[7] = 0x30; f[8] = 0x55;
    std::copy(payload.begin(), payload.end(), f.begin() + 10);
    levoit_finalize_message(f, 7);
    rig.feed(f, now);
  };
  report({0x00, 1, 1, 0x02, 1, 0}, 1000);
  CHECK(rig.host.fan_power == 0);
  report({0x00, 1, 1, 0x02, 1, 1}, 2000);
  CHECK(rig.host.fan_power == 1);
  report({0x00, 1, 1, 0x02, 1, 0}, 3000);
  CHECK(rig.host.fan_power == 0);
  report({0x00, 1, 1}, 4000); // no power field: keep the last state
  CHECK(rig.host.fan_power == 0);
  report({0x02, 1, 2}, 5000); // invalid enum: do not invent a state
  CHECK(rig.host.fan_power == 0);
  report({0x02, 2, 1, 0}, 6000); // invalid width
  CHECK(rig.host.fan_power == 0);

  rig.platform.tx.clear();
  rig.device.on_fan_command(1, 1, 0);
  CHECK(rig.platform.tx.size() == 3);
  if (rig.platform.tx.size() == 3) {
    // Power, Manual, speed 1, in that order; compare command and payload.
    const Bytes commands[] = {{2, 0, 0x50}, {2, 0x32, 0x55}, {2, 0x33, 0x55}};
    for (size_t i = 0; i < 3; i++) {
      const auto &f = rig.platform.tx[i];
      CHECK(f.size() == 13);
      if (f.size() == 13) {
        CHECK(Bytes(f.begin() + 6, f.begin() + 9) == commands[i]);
        CHECK(Bytes(f.begin() + 10, f.end()) == Bytes({1, 1, 1}));
        CHECK(f[5] == levoit_checksum(f.data(), f.size()));
      }
    }
  }
  rig.platform.tx.clear();
  rig.device.on_fan_command(0, -1, -1);
  CHECK(rig.platform.tx.size() == 1);
  if (rig.platform.tx.size() == 1)
    CHECK(rig.platform.tx[0].back() == 0);
  // An explicit retry must still reach the wire when core state is already off.
  rig.device.on_fan_command(0, -1, -1);
  CHECK(rig.platform.tx.size() == 2);
  report({0x00, 1, 1, 0x02, 1, 1}, 7000);
  CHECK(rig.host.fan_power == 1); // MCU still running after the off request
  report({0x00, 1, 1, 0x02, 1, 0}, 8000);
  CHECK(rig.host.fan_power == 0);
}

int main() {
  test_superior_power_and_commands();
  test_core300s_capture();
  test_command();
  test_dedup();
  test_refused_command();
  test_capabilities();
  test_set_filter_percent();
  test_begin_and_tracking();
  test_filter_accounting();
  test_wifi_led();
  test_filter_led();
  test_acks();
  test_version_guard();
  test_parser();
  if (failures == 0)
    std::printf("levoit device core: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
