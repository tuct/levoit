#include "../protocol.h"
#include <cassert>
#include <iostream>
using namespace esphome::lv600s_humidifier;
void feed(Protocol &d, const Bytes &f, uint32_t now = 100) { for (auto b : f) d.feed(b, now); }
Bytes state(bool on = true) {
  Bytes s(20); s[0] = 56; s[2] = 1; s[3] = on; s[6] = 100; s[8] = 55;
  s[9] = 42; s[10] = 23; s[11] = 3; s[12] = 4; s[13] = 2;
  return s;
}
void verify_recorded_water_depletion() {
  // MCU payloads from the 1.1.08-mcu-feedback water-depletion run. The API
  // samples did not retain the opcode or header, so packet() provides a
  // synthetic envelope. The intervals below are for replay, not wire timing.
  Protocol device;
  const Bytes connected{0x38, 0x00, 0x01, 0x00, 0x00, 0x00, 0x64, 0x01, 0x64, 0x24,
                        0x12, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const Bytes misting{0x38, 0x00, 0x01, 0x01, 0x00, 0x00, 0x64, 0x00, 0x64, 0x24,
                        0x12, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const Bytes low_water{0x38, 0x00, 0x01, 0x01, 0x00, 0x01, 0x64, 0x01, 0x64, 0x46,
                        0x10, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const Bytes standby{0x38, 0x00, 0x01, 0x00, 0x00, 0x01, 0x64, 0x01, 0x64, 0x46,
                        0x10, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  feed(device, packet(Command::REPORT, connected, 0), 100);
  assert(device.status.size == 20 && device.status.safe() && !device.status.power());
  assert(!device.status.humidifying() && !device.status.display());
  feed(device, packet(Command::REPORT, misting, 1), 200);
  assert(device.status.power() && device.status.humidifying() && device.status.display());
  feed(device, packet(Command::REPORT, low_water, 2), 300);
  assert(device.status.power() && !device.status.tank_removed() && device.status.water_empty());
  assert(!device.status.humidifying() && !device.status.safe());
  feed(device, packet(Command::REPORT, standby, 3), 400);
  assert(!device.status.power() && !device.status.display() && device.status.water_empty());
  assert(device.status.raw[6] == 100 && device.status.raw[12] == 9 && device.status.raw[15] == 0);
}
void verify_observed_standby_reply_shapes() {
  Protocol device;
  std::vector<Bytes> sent;
  device.transmit = [&](const Bytes &frame) { sent.push_back(frame); };

  auto dry = state(false);
  dry[5] = 1;
  dry[7] = 1;
  dry[8] = 100;
  dry[11] = 1;
  dry[12] = 5;
  dry[13] = 3;

  device.query();
  device.query_timer();
  assert(sent.size() == 2);
  assert(sent[0] == packet(Command::STATUS, {}, 0, 0x22));
  assert(sent[1] == packet(Command::TIMER_QUERY, {}, 1, 0x22));
  // The observed MCU sometimes returned the next sequence for a status reply.
  feed(device, packet(Command::STATUS, dry, 1, 0x12), 100);
  assert(device.status.fresh(100) && device.status.size == 20);
  assert(!device.status.power() && !device.status.display() && device.status.water_empty());
  assert(!device.status.tank_removed() && !device.status.humidifying() && !device.status.heating_enabled());
  assert(device.status.raw[12] == 5 && device.status.raw[13] == 3);
  feed(device, packet(Command::TIMER_QUERY, {}, 1, 0x12), 110);
  assert(!device.timer_known(110) && std::isnan(device.remaining(110)));

  assert(device.enqueue(Command::POWER, {0}, 200));
  assert(sent.size() == 3 && sent.back() == packet(Command::POWER, {0}, 2, 0x22));
  feed(device, packet(Command::POWER, {}, 2, 0x12), 220);
  assert(std::string(device.result) == "Sent: waiting for MCU target state");
  feed(device, packet(Command::REPORT, dry, 0x1A, 0x02), 225);
  assert(std::string(device.result) == "Target confirmed by MCU");
}
void verify_observed_sleep_confirmation() {
  Protocol device;
  const Bytes manual{0x38, 0x00, 0x01, 0x01, 0x00, 0x00, 0x64, 0x00, 0x64, 0x2e,
                     0x13, 0x01, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const Bytes sleep{0x38, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x64, 0x2e,
                    0x13, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  feed(device, packet(Command::STATUS, manual, 0xAA, 0x12), 100);
  assert(device.enqueue(Command::SLEEP, {55, 0, 0, 0, 0, 0}, 200));
  feed(device, packet(Command::SLEEP, {}, 0xAC, 0x12), 250);
  feed(device, packet(Command::STATUS, manual, 0xAD, 0x12), 260);
  auto different_target = sleep;
  different_target[8] = 60;
  feed(device, packet(Command::REPORT, different_target, 0x43, 0x02), 275);
  assert(std::string(device.result) == "Sent: waiting for MCU target state");
  feed(device, packet(Command::REPORT, sleep, 0x44, 0x02), 300);
  assert(std::string(device.result) == "Target confirmed by MCU");
  device.tick(2800);
  assert(device.command_failures == 0 && !device.status.display());
}
void verify_observed_tank_removal() {
  Protocol device;
  const Bytes removed{0x38, 0x00, 0x01, 0x00, 0x01, 0x00, 0x64, 0x01, 0x64, 0x21,
                      0x15, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  feed(device, packet(Command::REPORT, removed, 0, 0x02), 100);
  assert(device.status.tank_removed() && !device.status.water_empty());
  assert(!device.status.power() && !device.status.safe());
  assert(!device.enqueue(Command::POWER, {1}, 150));
  auto refilled = removed;
  refilled[4] = 0;
  feed(device, packet(Command::REPORT, refilled, 1, 0x02), 200);
  assert(!device.status.tank_removed() && !device.status.water_empty() && device.status.safe());
}
void verify_observed_level_write_shapes() {
  // Use synthetic frames with the observed payload lengths and reply order.
  Protocol device;
  std::vector<Bytes> sent;
  device.transmit = [&](const Bytes &frame) { sent.push_back(frame); };
  auto status = state();
  status[11] = 1;
  status[12] = 5;
  status[13] = 0;
  feed(device, packet(Command::STATUS, status, 0, 0x12), 100);

  assert(device.enqueue(Command::MIST, {1, 0, 0, 0}, 200));
  assert(sent.back() == packet(Command::MIST, {1, 0, 0, 0}, 0, 0x22));
  feed(device, packet(Command::MIST, {}, 0, 0x12), 230);
  assert(std::string(device.result) == "Sent: waiting for MCU target state");
  status[12] = 1;
  feed(device, packet(Command::REPORT, status, 1, 0x02), 285);
  assert(std::string(device.result) == "Target confirmed by MCU");

  assert(device.enqueue(Command::WARM, {1, 0, 0, 0, 0}, 300));
  assert(sent.back() == packet(Command::WARM, {1, 0, 0, 0, 0}, 1, 0x22));
  feed(device, packet(Command::WARM, {}, 1, 0x12), 330);
  assert(std::string(device.result) == "Sent: waiting for MCU target state");
  status[13] = 1;
  feed(device, packet(Command::REPORT, status, 2, 0x02), 380);
  assert(std::string(device.result) == "Target confirmed by MCU");
  assert(device.command_failures == 0 && device.status.raw[12] == 1 && device.status.heating_enabled());
}
int main() {
  verify_recorded_water_depletion();
  verify_observed_standby_reply_shapes();
  verify_observed_sleep_confirmation();
  verify_observed_tank_removal();
  verify_observed_level_write_shapes();
  Protocol d; std::vector<Bytes> sent; d.transmit = [&](const Bytes &f) { sent.push_back(f); };
  feed(d, packet(Command::STATUS, state(), 0, 0x12));
  assert(d.status.fresh(100) && d.status.raw[9] == 42 && d.status.heating_enabled());
  feed(d, packet(Command::REPORT, state(false), 1, 0x02), 200);
  assert(!d.status.heating_enabled() && !d.status.display() && !d.status.humidifying());
  auto count = d.revision; auto bad = packet(Command::REPORT, state(), 2);
  bad[9] = 1; bad[5] = checksum(bad); feed(d, bad); assert(d.revision == count);
  bad[9] = 0; bad[6] = 2; bad[5] = checksum(bad); feed(d, bad); assert(d.revision == count);
  feed(d, packet(Command::REPORT, Bytes(19), 2)); assert(d.revision == count);
  auto extended = state();
  extended[19] = 42;
  feed(d, packet(Command::REPORT, extended, 3));
  assert(d.status.size == 20 && d.status.raw[19] == 42);
  auto short_status = state(); short_status.resize(16);
  feed(d, packet(Command::STATUS, short_status, 4, 0x12));
  assert(d.revision == count + 2 && d.status.size == 16 && d.status.raw[19] == 0);
  // A valid ACK/error that happens to carry reset-looking bytes must not reset Wi-Fi.
  int buttons = 0; d.panel_event = [&](uint8_t) { ++buttons; };
  for (uint8_t ctrl : {0x02, 0x12, 0x22, 0x52})
    for (uint16_t code : {0xD102, 0xD103})
      for (unsigned action = 0; action < 256; ++action) {
        feed(d, packet(Command(code), {uint8_t(action)}, 9, ctrl));
      }
  assert(buttons == 6); // 02/22: D102/2, D102/3, D103/4 only
  // Fragmented >255-byte frames, high-byte and checksum corruption, stale fragments.
  Protocol stream; auto large = packet(Command::WIFI, Bytes(300, 0), 0);
  feed(stream, Bytes(large.begin(), large.begin() + 200));
  feed(stream, Bytes(large.begin() + 200, large.end()), 200);
  auto valid = packet(Command::REPORT, state(), 1);
  feed(stream, {0xA5, 0x22, 0, 0xF0, 0, 0}, 250); feed(stream, valid, 501);
  assert(stream.revision == 1);
  for (auto corrupt : {Bytes{0xA5, 0x22, 0, 4, 0xFF, 0}, Bytes{0xA5, 0x22, 0, 0, 0, 0}}) {
    feed(stream, corrupt, 502); feed(stream, valid, 502);
  }
  assert(stream.revision == 3);
  bad = valid; bad[5] ^= 1; feed(stream, bad, 503); feed(stream, valid, 503); assert(stream.revision == 4);
  Protocol wrap; feed(wrap, {0xA5, 0x22}, 0xFFFFFFF0); feed(wrap, valid, 300); assert(wrap.revision == 1);
  // Off is always available, other setters must not run with missing/unsafe status.
  Protocol off; std::vector<Bytes> off_tx; off.transmit = [&](const Bytes &f) { off_tx.push_back(f); };
  assert(!off.enqueue(Command::POWER, {1}, 0));
  assert(off.enqueue(Command::POWER, {0}, 0)); assert(off_tx[0] == packet(Command::POWER, {0}, 0));
  assert(!d.enqueue(Command::TARGET, {0, 39}, 200));
  assert(d.enqueue(Command::DISPLAY, {0}, 200));
  feed(d, packet(Command::DISPLAY, {}, 1, 0x12), 250);
  assert(std::string(d.result) == "Sent: waiting for MCU target state");
  auto st = state(); st[6] = 0; feed(d, packet(Command::REPORT, st, 2), 300);
  assert(std::string(d.result) == "Target confirmed by MCU");
  // A timer write is never retried; status ACK alone does not confirm it.
  assert(d.enqueue(Command::TIMER, {60, 0, 0, 0}, 301));
  d.tick(2801); assert(d.command_failures == 2); // earlier rejected target + timer timeout
  feed(d, packet(Command::TIMER_REPORT, {59, 0, 0, 0, 60, 0, 0, 0}, 3, 0x02), 3000);
  assert(d.remaining(4000) == 58);
  feed(d, packet(Command::TIMER_REPORT, {59, 0, 0, 0, 60, 0, 0, 0}, 3, 0x02), 4000);
  feed(d, packet(Command::TIMER_QUERY, {}, 4, 0x12), 4350);
  assert(d.remaining(4350) == 58); // an empty timer reply leaves the countdown intact
  assert(d.remaining(5000) == 57); // duplicate snapshots do not restart countdown
  feed(d, packet(Command::TIMER_REPORT, {61, 0, 0, 0, 60, 0, 0, 0}, 4, 0x02), 5000);
  assert(d.remaining(5000) == 57);
  feed(d, packet(Command::TIMER_REPORT, {0, 0, 0, 0, 60, 0, 0, 0}, 5, 0x02), 5100);
  assert(d.remaining(5100) == 0 && std::isnan(d.remaining(20000)));
  // The EU MCU can report only at start/expiry. Fresh ordinary status keeps
  // the countdown available but cannot confirm zero or revive a stale link.
  Protocol sparse;
  feed(sparse, valid, 1000);
  feed(sparse, packet(Command::TIMER_REPORT, {60, 0, 0, 0, 60, 0, 0, 0}, 1), 1000);
  for (uint32_t t = 6000; t <= 71000; t += 5000) feed(sparse, valid, t);
  assert(std::isnan(sparse.remaining(71000)));

  Protocol countdown;
  feed(countdown, valid, 1000);
  feed(countdown, packet(Command::TIMER_REPORT, {60, 0, 0, 0, 60, 0, 0, 0}, 1), 1000);
  for (uint32_t t = 6000; t <= 61000; t += 5000) {
    feed(countdown, valid, t);
    const auto elapsed = (t - 1000) / 1000;
    assert(countdown.remaining(t) == (elapsed < 60 ? 60 - elapsed : 1));
  }
  feed(countdown, packet(Command::TIMER_REPORT, {0, 0, 0, 0, 60, 0, 0, 0}, 2), 61500);
  assert(countdown.remaining(61500) == 0);
  feed(countdown, valid, 66000);
  feed(countdown, valid, 71000);
  feed(countdown, valid, 76000);
  assert(countdown.remaining(78000) == 0);
  feed(countdown, valid, 92000);
  assert(std::isnan(countdown.remaining(92000)));
  countdown.enqueue(Command::TIMER, {1, 0, 0, 0}, 92001);
  feed(countdown, packet(Command::TIMER_QUERY, {0, 0}, 3, 0x12), 92020);
  assert(std::string(countdown.result) == "Sent: waiting for MCU target state");
  feed(countdown, packet(Command::TIMER_QUERY, {1, 0}, 4, 0x12), 92030);
  assert(std::string(countdown.result) == "Target confirmed by MCU");

  // Safety changes are rechecked before the next queued command is sent.
  Protocol q; std::vector<Bytes> safety_tx;
  q.transmit = [&](const Bytes &f) { safety_tx.push_back(f); };
  feed(q, valid, 100); q.enqueue(Command::MIST, {5, 0, 0, 0}, 101);
  q.enqueue(Command::WARM, {3, 0, 0, 0, 0}, 102);
  st = state(); st[4] = 1; st[12] = 5; feed(q, packet(Command::REPORT, st, 1), 200);
  assert(q.command_failures == 2 && !q.status.heating_enabled());
  feed(q, valid, 201); q.tick(201);
  assert(safety_tx.size() == 1);  // fresh status in the same burst cannot revive either write

  Protocol disconnected;
  disconnected.transmit = [&](const Bytes &f) { safety_tx.push_back(f); };
  feed(disconnected, valid, 100);
  disconnected.enqueue(Command::WARM, {3, 0, 0, 0, 0}, 15090);
  feed(disconnected, valid, 15100); disconnected.tick(17600);
  assert(disconnected.command_failures == 1 && safety_tx.size() == 2);

  // Known and unknown MCU faults must follow the same write restrictions.
  for (const uint8_t fault : {1, 2, 3, 4, 5, 6, 255}) {
    Protocol faulted;
    auto status = state();
    status[15] = fault;
    feed(faulted, packet(Command::REPORT, status, 1), 100);
    assert(!faulted.status.humidifying() && !faulted.status.heating_enabled());
    assert(!faulted.enqueue(Command::POWER, {1}, 101));
    assert(!faulted.enqueue(Command::WARM, {3, 0, 0, 0, 0}, 101));
    assert(faulted.enqueue(Command::POWER, {0}, 101));
  }
  Protocol dry;
  auto dry_status = state();
  dry_status[5] = 1;
  feed(dry, packet(Command::REPORT, dry_status, 1), 100);
  assert(!dry.enqueue(Command::MIST, {5, 0, 0, 0}, 101));
  assert(dry.enqueue(Command::POWER, {0}, 101));

  // Explicit OFF cancels queued setters and is the next actual write.
  assert(q.enqueue(Command::POWER, {0}, 202));
  assert(q.enqueue(Command::WARM, {1, 0, 0, 0, 0}, 203));
  feed(q, packet(Command::REPORT, st, 2), 204);
  assert(safety_tx.back()[7] == 0 && safety_tx.back()[8] == 0xA0);
  auto standby = state(false); standby[4] = 1;
  feed(q, packet(Command::REPORT, standby, 3), 205);
  assert(std::string(q.result) == "Target confirmed by MCU");
  std::cout << "LV600S protocol, command confirmation, safety, timer and reset guards passed\n";
}
