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

#include <cstdio>
#include <cstring>
#include <fstream>
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

// Full captured ESP RX frames from devices/levoit-lv600s/uart/. Each
// status updates the shared state, including 0x02 pushes between polls.
static void test_lv600s_captures() {
  unsigned frames = 0, statuses = 0;
  unsigned mist_levels = 0, warm_levels = 0;
  for (const char *name : {"poll-standby.csv", "panel-mode-timer.csv", "api-control-feedback.csv"}) {
    const auto path = std::string("devices/levoit-lv600s/uart/") + name;
    std::ifstream input(path);
    CHECK(input.is_open());
    if (!input) continue;
    std::string line;
    std::getline(input, line);  // CSV header
    Rig rig("LV600S");
    uint32_t now = 0;
    while (std::getline(input, line)) {
      std::istringstream row(line);
      std::string test_case, elapsed, direction, opcode, data;
      std::getline(row, test_case, ',');
      std::getline(row, elapsed, ',');
      std::getline(row, direction, ',');
      std::getline(row, opcode, ',');
      std::getline(row, data);
      frames++;
      const auto packet = hex(data.c_str());
      CHECK(packet.size() >= 10 && levoit_checksum(packet.data(), packet.size()) == packet[5]);
      if (direction == "TX") continue;
      CHECK(direction == "RX");
      const bool status = opcode == "4110" || opcode == "4111";
      const int reports = rig.host.fan_reports;
      rig.feed(packet, ++now);
      CHECK(rig.host.fan_reports == reports + (status ? 1 : 0));
      if (!status || packet.size() != 30) continue;
      statuses++;
      const auto *p = packet.data() + 10;
      CHECK(rig.host.texts[TextSensorType::MCU_VERSION] == "1.0.56");
      CHECK(rig.host.fan_power == p[3] && rig.host.fan_speed == p[12]);
      CHECK(rig.host.binaries[BinarySensorType::TANK_REMOVED] == (p[4] == 1));
      CHECK(rig.host.binaries[BinarySensorType::WATER_TANK_EMPTY] == (p[5] == 1));
      CHECK(rig.host.switches[SwitchType::DISPLAY] == (p[3] == 1 && p[6] == 100 && p[11] != 2));
      CHECK(rig.host.sensors[SensorType::HUMIDITY] == p[9]);
      CHECK(rig.host.sensors[SensorType::TEMPERATURE] == p[10]);
      CHECK(rig.host.numbers[NumberType::WARM_LEVEL] == p[13]);
      if (p[12] <= 9) mist_levels |= 1u << p[12];
      if (p[13] <= 3) warm_levels |= 1u << p[13];
      if (test_case == "humidity-sleep-manual" && p[11] == 3) {
        CHECK(rig.host.fan_mode == 3 && rig.host.texts[TextSensorType::REPORTED_MODE] == "Humidity");
        CHECK(rig.host.numbers[NumberType::HUMIDITY_TARGET] == p[8]);
      }
      if (test_case == "humidity-sleep-manual" && p[11] == 2)
        CHECK(rig.host.fan_mode == 1 && rig.host.texts[TextSensorType::REPORTED_MODE] == "Sleep");
    }
    CHECK(rig.platform.tx.empty());
  }
  CHECK(frames == 90 && statuses == 42);
  constexpr unsigned mist_presets = (1u << 1) | (1u << 5) | (1u << 9);
  CHECK((mist_levels & mist_presets) == mist_presets);
  CHECK(warm_levels == 0x0F);
}

static void test_lv600s_status_guard() {
  // Panel warmth push at 2230 ms in panel-mode-timer.csv.
  const auto good = hex("A5 02 30 18 00 77 01 11 41 00 38 00 01 01 00 00 64 00 64 29 14 01 05 01 00 00 00 00 00 00");
  Rig rig("LV600S");
  auto bad_result = good;
  bad_result[9] = 1;
  levoit_finalize_message(bad_result, good[2]);
  rig.feed(bad_result, 10);
  auto bad_version = good;
  bad_version[6] = 2;
  levoit_finalize_message(bad_version, good[2]);
  rig.feed(bad_version, 11);
  auto short_push = good;
  short_push.resize(26);
  short_push[3] = 20;
  levoit_finalize_message(short_push, good[2]);
  rig.feed(short_push, 12);
  CHECK(rig.host.fan_reports == 0);

  // The standalone component also accepts 16-byte poll replies.
  auto short_reply = short_push;
  short_reply[1] = 0x12;
  short_reply[7] = 0x10;
  levoit_finalize_message(short_reply, good[2]);
  rig.feed(short_reply, 13);
  CHECK(rig.host.fan_reports == 1 && rig.host.fan_mode == 0);

  rig.feed(good, 14);
  CHECK(rig.host.fan_reports == 2 && rig.host.binaries[BinarySensorType::HEATING_ENABLED]);
  // Same status is still received by the LV600S model; normal typed slots deduplicate.
  rig.feed(good, 15);
  CHECK(rig.host.fan_reports == 3);
  auto original_auto = good;
  original_auto[1] = 0x22;
  original_auto[21] = 0;
  original_auto[25] = 3;
  levoit_finalize_message(original_auto, good[2]);
  rig.feed(original_auto, 16);
  CHECK(rig.host.fan_mode == 0 && rig.host.texts[TextSensorType::REPORTED_MODE] == "Auto (Original)");
  CHECK(rig.host.binaries[BinarySensorType::MCU_FAULT]);
  CHECK(!rig.host.binaries[BinarySensorType::HEATING_ENABLED] && !rig.host.binaries[BinarySensorType::HUMIDIFYING]);
  CHECK(rig.platform.tx.empty());

  Rig purifier("CORE600S");
  purifier.feed(good, 17);
  CHECK(purifier.host.fan_reports == 0 && purifier.host.sensors.count(SensorType::HUMIDITY) == 0);
}

static void test_lv600s_shared_commands() {
  Rig rig("LV600S");
  auto &device = rig.device;
  auto &controls = device.lv600s();
  device.begin();
  CHECK(rig.platform.tx.size() == 2);
  CHECK(rig.platform.last_with(0x10, 0x41) && rig.platform.last_with(0x65, 0xA2));

  // Status and timer ACK/report from panel-mode-timer.csv. Modify the
  // decoded standby and water flags to exercise the guard deterministically.
  auto status = hex("A5 12 EE 18 00 A3 01 10 41 00 38 00 01 01 00 00 64 00 64 32 13 01 05 00 00 00 00 00 00 00");
  auto report = [&](size_t offset, uint8_t value, uint32_t now) {
    status[10 + offset] = value;
    levoit_finalize_message(status, status[2]);
    rig.feed(status, now);
  };
  report(3, 0, 100);
  CHECK(rig.host.binaries[BinarySensorType::MCU_CONNECTION]);
  const auto poll_count = rig.platform.tx.size();
  device.on_fan_command(-1, 5, -1);
  device.on_fan_command(-1, -1, 3);
  device.on_fan_command(-1, 9, -1);
  CHECK(rig.platform.tx.size() == poll_count + 1 && rig.platform.last_with(0x00, 0xA0));
  rig.feed(lv600s::packet(lv600s::Command::POWER, {}, 0, 0x12), 105);
  device.tick(105);
  CHECK(!rig.platform.last_with(0x60, 0xA2) && !rig.platform.last_with(0x14, 0x41));
  CHECK(rig.host.fan_power == 0);
  report(3, 1, 110);
  device.tick(110);
  const auto *mist = rig.platform.last_with(0x60, 0xA2);
  CHECK(mist && (*mist)[12] == 9 && !rig.platform.last_with(0x14, 0x41));
  CHECK(rig.host.fan_speed == 5);
  report(12, 9, 120);
  CHECK(rig.host.fan_speed == 9 && rig.host.sensors[SensorType::MIST_LEVEL] == 9);
  device.on_switch_command(SwitchType::DISPLAY, false);
  CHECK(rig.platform.last_with(0x05, 0xA1));
  CHECK(rig.host.switches[SwitchType::DISPLAY]);
  report(6, 0, 130);
  CHECK(!rig.host.switches[SwitchType::DISPLAY]);

  device.on_fan_command(-1, -1, 3);
  const auto *humidity = rig.platform.last_with(0x14, 0x41);
  CHECK(humidity && (*humidity)[10] == 55);
  CHECK(rig.host.fan_mode == 0);
  report(8, 55, 131);
  report(11, 3, 132);
  CHECK(rig.host.fan_mode == 3);
  device.on_number_command(NumberType::HUMIDITY_TARGET, 66);
  const auto *target = rig.platform.last_with(0xE8, 0xA2);
  CHECK(target && (*target)[11] == 66);
  CHECK(rig.host.numbers[NumberType::HUMIDITY_TARGET] == 55);
  report(8, 66, 133);
  CHECK(rig.host.numbers[NumberType::HUMIDITY_TARGET] == 66);
  device.on_number_command(NumberType::WARM_LEVEL, 2);
  const auto *warm = rig.platform.last_with(0x12, 0x41);
  CHECK(warm && (*warm)[10] == 2);
  CHECK(rig.host.numbers[NumberType::WARM_LEVEL] == 0);
  report(13, 2, 134);
  CHECK(rig.host.numbers[NumberType::WARM_LEVEL] == 2);
  device.on_fan_command(-1, -1, 1);
  const auto *sleep = rig.platform.last_with(0x82, 0x40);
  CHECK(sleep && (*sleep)[10] == 66);
  report(11, 2, 135);
  CHECK(rig.host.fan_mode == 1);

  uint32_t timer_id = 0;
  CHECK(controls.set_timer_seconds(60, &timer_id));
  const auto *timer = rig.platform.last_with(0x64, 0xA2);
  CHECK(timer && (*timer)[10] == 60);
  rig.feed(hex("A5 12 F0 04 00 4D 01 64 A2 00"), 140);
  CHECK(!controls.timer_write_confirmed(timer_id));
  const auto running_timer = hex("A5 22 59 0C 00 52 01 66 A2 00 3C 00 00 00 3C 00 00 00");
  const auto cleared_timer = hex("A5 22 5B 0C 00 8C 01 66 A2 00 00 00 00 00 3C 00 00 00");
  rig.feed(running_timer, 150);
  CHECK(controls.timer_write_confirmed(timer_id) && controls.get_timer_duration() == 60);
  rig.platform.now = 1150;
  device.tick(1150);
  CHECK(controls.get_timer_remaining() == 59);
  CHECK(rig.host.texts[TextSensorType::TIMER_DURATION_CURRENT] == "1 min");
  CHECK(rig.host.numbers[NumberType::TIMER] == 1);
  device.on_number_command(NumberType::TIMER, 0);
  timer = rig.platform.last_with(0x64, 0xA2);
  CHECK(timer && (*timer)[10] == 0);
  rig.feed(cleared_timer, 1200);
  CHECK(controls.get_timer_remaining() == 0 && controls.get_timer_duration() == 0);

  report(5, 1, 1250);
  const auto before_rejected = rig.platform.tx.size();
  device.on_number_command(NumberType::WARM_LEVEL, 3);
  device.on_fan_command(-1, 1, 0);
  CHECK(rig.platform.tx.size() == before_rejected);
  device.on_fan_command(0, -1, -1);
  CHECK(rig.platform.tx.size() == before_rejected + 1);
  report(3, 0, 1300);
  rig.platform.now = 16400;
  device.tick(16400);
  CHECK(!rig.host.binaries[BinarySensorType::MCU_CONNECTION]);
  CHECK(std::isnan(rig.host.sensors[SensorType::HUMIDITY]));
  CHECK(rig.host.texts[TextSensorType::TIMER_DURATION_CURRENT] == "Unknown");
  CHECK(std::isnan(rig.host.numbers[NumberType::TIMER]));
  CHECK(rig.platform.acks() == 0);

  report(3, 1, 16500);
  rig.feed(running_timer, 16600);
  rig.platform.now = 17400;
  device.tick(17400);
  CHECK(rig.host.numbers[NumberType::TIMER] == 1);

  // The MCU is still reachable, but the expiry confirmation never arrived.
  report(3, 1, 86600);
  device.tick(86600);
  CHECK(rig.host.binaries[BinarySensorType::MCU_CONNECTION]);
  CHECK(rig.host.texts[TextSensorType::TIMER_DURATION_CURRENT] == "Unknown");
  CHECK(std::isnan(rig.host.numbers[NumberType::TIMER]));

  rig.feed(cleared_timer, 86700);
  rig.platform.now = 87600;
  device.tick(87600);
  CHECK(rig.host.texts[TextSensorType::TIMER_DURATION_CURRENT] == "Off");
  CHECK(rig.host.numbers[NumberType::TIMER] == 0);
}

static void test_lv600s_deferred_fan_cancelled() {
  // Same fixed status layout as panel-mode-timer.csv, with synthetic power
  // and water changes during a wake-up.
  const auto frame = hex("A5 12 EE 18 00 A3 01 10 41 00 38 00 01 01 00 00 64 00 64 32 13 01 05 00 00 00 00 00 00 00");
  enum class Abort { POWER_OFF, WATER_SHORTAGE, UNSAFE_STATUS_BURST, TIMEOUT };
  for (const auto cause : {Abort::POWER_OFF, Abort::WATER_SHORTAGE, Abort::UNSAFE_STATUS_BURST, Abort::TIMEOUT}) {
    Rig rig("LV600S");
    auto status = frame;
    status[13] = 0;
    levoit_finalize_message(status, status[2]);
    rig.feed(status, 1);
    if (cause == Abort::WATER_SHORTAGE)
      rig.device.on_fan_command(-1, 9, -1);
    else
      rig.device.on_fan_command(-1, -1, 1);
    CHECK(rig.platform.last_with(0x00, 0xA0));
    if (cause == Abort::POWER_OFF) {
      rig.device.on_fan_command(0, -1, -1);
    } else if (cause == Abort::WATER_SHORTAGE || cause == Abort::UNSAFE_STATUS_BURST) {
      if (cause == Abort::UNSAFE_STATUS_BURST) rig.feed(frame, 2); // Wake confirmed before the unsafe report
      status[15] = 1;
      levoit_finalize_message(status, status[2]);
      rig.feed(status, 3);
    } else {
      rig.platform.now = 5002;
      rig.device.tick(2502);
      rig.device.tick(5002);
    }
    const uint32_t now = rig.platform.now + 1;
    status[13] = 1;
    status[15] = 0;
    levoit_finalize_message(status, status[2]);
    rig.feed(status, now);
    rig.device.tick(now);
    CHECK(!rig.platform.last_with(0x82, 0x40) && !rig.platform.last_with(0x60, 0xA2));
  }
}

int main() {
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
  test_lv600s_captures();
  test_lv600s_status_guard();
  test_lv600s_shared_commands();
  test_lv600s_deferred_fan_cancelled();
  if (failures == 0)
    std::printf("levoit device core: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
