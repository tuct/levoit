#include <cstdio>
#include <cstring>
#include <vector>
#include "../../components/levoit/mcu_dfu.h"
#include "../../components/levoit/levoit_message.h"

using namespace levoit;

struct FakePlatform : LevoitPlatform {
  std::vector<std::vector<uint8_t>> tx;
  uint32_t now = 1000;
  void write(const uint8_t *data, size_t len) override { tx.emplace_back(data, data + len); }
  uint32_t millis() override { return now; }
  void set_timeout(const char *, uint32_t, std::function<void()>) override {}
  void cancel_timeout(const char *) override {}
  bool load_u32(const char *, uint32_t &) override { return false; }
  void save_u32(const char *, uint32_t) override {}
  bool has_clock() override { return false; }
  bool local_time(uint32_t &) override { return false; }
  NetworkState network_state() override { return NetworkState::CONNECTED; }
};

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %d: %s\n", __LINE__, #c); failures++; } } while (0)

static std::vector<uint8_t> mcu_request(uint8_t seq, uint8_t op_lo, uint8_t op_hi,
                                        const std::vector<uint8_t> &payload) {
  // A5 22 seq len len chk 01 op_lo op_hi 00 payload
  std::vector<uint8_t> f = {0xA5, 0x22, seq, 0, 0, 0, 0x01, op_lo, op_hi, 0x00};
  f.insert(f.end(), payload.begin(), payload.end());
  uint16_t len = (uint16_t)(f.size() - 6);
  f[3] = (uint8_t)len; f[4] = (uint8_t)(len >> 8);
  f[5] = levoit_checksum(f.data(), f.size());
  return f;
}
static std::vector<uint8_t> mcu_ack(uint8_t seq, uint8_t op_lo, uint8_t op_hi,
                                    const std::vector<uint8_t> &payload) {
  auto f = mcu_request(seq, op_lo, op_hi, payload); f[1] = 0x12; f[5] = levoit_checksum(f.data(), f.size()); return f;
}

int main() {
  FakePlatform p;
  McuDfu dfu;
  dfu.begin(&p);
  uint8_t image[0x1000];
  for (size_t i = 0; i < sizeof(image); i++) image[i] = (uint8_t)(i * 7);
  CHECK(dfu.start([&](uint32_t off, uint8_t *dst, size_t len) -> size_t {
    if (off >= sizeof(image)) return 0;
    size_t n = len; if (sizeof(image) - off < n) n = sizeof(image) - off;
    memcpy(dst, image + off, n); return n;
  }, sizeof(image), 1, 0, 5));
  CHECK(p.tx.size() == 1);
  // start frame: A5 22 10 0F 00 chk 01 30 80 00 | 00 04 01 00 05 00 10 00 00 FF FF
  const auto &s = p.tx[0];
  CHECK(s.size() == 21);
  CHECK(s[0] == 0xA5 && s[1] == 0x22 && s[6] == 0x01 && s[7] == 0x30 && s[8] == 0x80 && s[9] == 0x00);
  CHECK(s[10] == 0x00 && s[11] == 0x04 && s[12] == 1 && s[13] == 0 && s[14] == 5);
  CHECK(s[15] == 0x00 && s[16] == 0x10 && s[17] == 0x00 && s[18] == 0x00 && s[19] == 0xFF && s[20] == 0xFF);
  CHECK(dfu.state() == McuDfu::State::WAIT_READY);

  // MCU ready: status 0, MTU 512
  auto a = mcu_ack(0x10, 0x30, 0x80, {0x00, 0x00, 0x02});
  CHECK(dfu.handle_frame(a[1], a[2], a[7], a[8], a.data() + 10, a.size() - 10));
  CHECK(dfu.state() == McuDfu::State::TRANSFER);
  CHECK(dfu.mtu() == 512);

  // first chunk pull
  auto r = mcu_request(0x11, 0x31, 0x80, {0x00, 0x00, 0x00, 0x00});
  CHECK(dfu.handle_frame(r[1], r[2], r[7], r[8], r.data() + 10, r.size() - 10));
  CHECK(p.tx.size() == 2);
  const auto &c = p.tx[1];
  CHECK(c[1] == 0x12 && c[2] == 0x11 && c[7] == 0x31 && c[8] == 0x80);
  CHECK(c[10] == 0x00);                 // not last
  CHECK(c[11] == 0 && c[12] == 0 && c[13] == 0 && c[14] == 0);
  CHECK(c[15] == 0x00 && c[16] == 0x02); // len 512
  CHECK(c.size() == 10 + 7 + 512);
  CHECK(memcmp(c.data() + 17, image, 512) == 0);

  // last chunk
  auto r2 = mcu_request(0x12, 0x31, 0x80, {0x00, 0x0F, 0x00, 0x00}); // 0xF00
  CHECK(dfu.handle_frame(r2[1], r2[2], r2[7], r2[8], r2.data() + 10, r2.size() - 10));
  const auto &c2 = p.tx.back();
  CHECK(c2[10] == 0x01);               // last
  CHECK(dfu.progress() == 99);

  // verified + burned + reboot
  auto v = mcu_request(0x13, 0x32, 0x80, {0x00});
  CHECK(dfu.handle_frame(v[1], v[2], v[7], v[8], v.data() + 10, v.size() - 10));
  CHECK(dfu.state() == McuDfu::State::BURNING);
  auto b = mcu_request(0x14, 0x33, 0x80, {0x00});
  CHECK(dfu.handle_frame(b[1], b[2], b[7], b[8], b.data() + 10, b.size() - 10));
  CHECK(dfu.state() == McuDfu::State::DONE);
  const auto &reb = p.tx.back();
  CHECK(reb[6] == 0x01 && reb[7] == 0x01 && reb[8] == 0xD1 && reb[10] == 0x01);

  // timeout
  McuDfu d2; FakePlatform p2; d2.begin(&p2);
  CHECK(d2.start([&](uint32_t, uint8_t *, size_t) -> size_t { return 0; }, 16, 1, 0, 0));
  d2.tick(p2.now + 20000);
  CHECK(d2.state() == McuDfu::State::FAILED);

  if (failures == 0) std::printf("all mcu_dfu checks passed\n");
  return failures ? 1 : 0;
}
