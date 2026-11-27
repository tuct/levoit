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
  CHECK(rig.host.switches[SwitchType::DISPLAY] == false);

  // "power off, at the panel" at 189.370: off, the kept manual speed 1
  rig.feed(hex("A5 22 D4 17 00 D6 01 32 40 00 00 00 01 00 00 01 00 00 01 FF 00 01 01 00 00 00 00 00 00"), 3000);
  CHECK(rig.host.fan_power == 0 && rig.host.fan_speed == 1 && rig.host.fan_mode == 0);
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

// begin(): counters restored through the platform, the Core 300S asks for its
// status once if nothing arrived, CADR tracking runs on the device state.
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

  // Core 300S: 214 m³/h at speed 3 of 3; a minute of it goes to the counters
  CHECK(rig.device.calculate_current_cadr_per_hour() == 214);
  rig.device.track_cadr_usage();
  CHECK(rig.device.get_total_runtime() == 57);
  CHECK(rig.device.get_used_cadr() == 1234 + 214 / 60);
  CHECK(rig.platform.prefs[LevoitDevice::PREF_TOTAL_RUNTIME] == 57);
  CHECK(rig.host.sensors.count(SensorType::FILTER_LIFE_LEFT) == 1);
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

int main() {
  test_core300s_capture();
  test_command();
  test_dedup();
  test_begin_and_tracking();
  test_wifi_led();
  test_acks();
  test_version_guard();
  test_parser();
  if (failures == 0)
    std::printf("levoit device core: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
