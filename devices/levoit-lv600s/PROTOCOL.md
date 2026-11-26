# LV600S protocol notes

Observed hardware: LUH-A602S-WEUR, ESP32-SOLO-1C, MCU 1.0.56.
MCU UART: ESP TX17/RX16, 9600 baud, 8N1. UART0 for programming is separate.
The Superior 6000S uses a different model/protocol and is not an LV600S alias.

## Framing

`A5 control sequence length_lo length_hi checksum version command_lo command_hi result payload...`

Length counts all bytes after the checksum (four header bytes + payload). The
checksum is the complement of the sum of all other bytes, modulo 256. LV replies
use version 0/1 and result 0; SEND controls are 02/22, reply control 12. A 250 ms
inter-byte timeout and a 512-byte body bound prevent a damaged length from
holding later reports indefinitely. This component does not assume that the
purifiers' ACK policy applies to LV600S; the prior working LV implementation did
not send automatic report ACKs either.

## Status

`0x4110` polls status. The decoder accepts 16- and 20-byte replies and
20-byte `0x4111` pushes. The assembled EU unit confirmed the push path. The raw
diagnostic shows the actual payload length, including any trailing bytes received.

| Offset | Meaning |
|---|---|
| 0..2 | MCU patch, minor, major |
| 3 | Power |
| 4 | Tank state, zero = present |
| 5 | Water shortage |
| 6 | Display setting, 0/100 |
| 7 | Fog state; zero participates in the derived humidifying state |
| 8 | Target RH. Manual and Sleep can report 100 when no adjustable 40–80% target is configured |
| 9 | Current RH |
| 10 | Temperature |
| 11 | Mode: 0 original Auto (read only), 1 Manual, 2 Humidity Sleep, 3 Humidity |
| 12 | Mist level 1..9 (internal scale; app groups into three levels) |
| 13 | Warm level 0..3, retained in standby |
| 14 | Uninterpreted diagnostic byte |
| 15 | MCU fault raw value; zero = no reported fault |
| 16..19 | Preserved raw diagnostics, no asserted meaning |

Heating-enabled and humidifying are **derived command states**, not measurements
of heater temperature or physical mist. Power, water, tank and fault state gate
both; warmth also requires a nonzero retained warm setting. Display is dark in
standby and Humidity Sleep regardless of the retained display setting. Status
older than 15 seconds is unavailable and cannot authorize an on/change command.

## Commands

| Command | Payload |
|---|---|
| A000 | Power 00/01 |
| A105 | Display 00/64 |
| A2E8 | 00, target RH 40..80 |
| 4113 | Mist 1..9, 00 00 00 |
| 4112 | Warm 0..3, 00 00 00 00 |
| A260 | Warm-enable, mist-enable, level (manual) |
| 4114 | RH target, 00 00 00 (humidity mode) |
| 4082 | RH target, 00 00 00 00 00 (sleep) |
| A264 | Timer seconds LE32, 0 cancels, max 43200 |
| A265 | Read remaining time; 2/4-byte LE reply |
| A266 | Report: remaining LE32, total LE32 |
| A129 | Wi-Fi pattern, on-ms LE16, off-ms LE16, repetitions |

A bounded eight-command queue confirms setters from later status reports, not
just ACKs. At most one retry for idempotent setters; timers are never retried.
The Sleep command requires mode 2 and accepts either the requested target or
the observed unconfigured value (100). Humidity mode and target writes still
require agreement with the requested target.
Explicit off cancels queued setters and remains available with missing status.
The release candidate discards pending setters when MCU status becomes stale
or reports a tank, water or fault condition. A subsequent good report does
not replay the discarded writes. The explicit off command remains pending
until confirmed or timed out. Repeated timer snapshots
do not restart the countdown. The EU MCU sometimes only sent a timer report at
start and expiry. Fresh ordinary status keeps the estimate available for the
reported duration plus a 10-second confirmation window. A zero report confirms
expiry or cancellation. A stale or resumed MCU connection requires a new timer
report. Following power-off from a timer, the MCU can report zero remaining
with the original total still present. Cancellation was observed as 0/0 with
the appliance still on.

## Power-button events

Only physical SEND 02/22 with successful result, one-byte payload and the exact
opcode/action pair is emitted: D102/2 or D102/3 (pairing), D103/4 (reset request).
ACKs, foreign envelopes and D102/4 are rejected. The callback only reports the
event. Erasing NVS/opening a provisioning AP remains the owner's configuration
policy; this component never erases flash or changes credentials by itself.

## Evidence and limits

The byte layout and commands were compared with an owner's previously working
LV600S deployment and analysis of that owner's firmware. This new implementation
is independently written; it does not vendor the separate, unlicensed
wdm230/levoit-lv600s-esphome source. That project is credited as prior work.
The initial component ran on the
assembled EU unit through the encrypted owner firmware. That run confirmed
power, display, mist, warmth, modes, target humidity, timer start and cancel.
The newer queue safety correction passed synthetic fault and recovery tests
and the real empty-tank check. The subsequent diagnostic run covered tank
removal, refill, wet operation and a Home Assistant timer at the same MCU
revision. [Release review](RELEASE-REVIEW.md)

### Recorded command events and water-depletion payloads

The owner's EU unit ran the original appliance MCU (1.0.56) with ESPHome
on the Wi-Fi module. The two recordings are distinct sessions on the same
9600-baud, 8N1 MCU link. RX and TX are from the ESP's point of view.
The archive matched all three checksums in its manifest.

| Source on 14 November 2026 | ESPHome application | Contents |
|---|---|---|
| `command-events.txt` | `1.1.06-panel-reset` | 33 events over 2 min 55.082 s. Each event has a log timestamp, direction, opcode and payload size. |
| `status-payloads.json` | `1.1.08-mcu-feedback` | 144 samples over 2 h 13 min 33.219 s, including repeated snapshots. Each carries a 20-byte MCU-to-ESP payload, a sample timestamp, and decoded power, humidifying, water-shortage and tank flags. |

| Direction | Opcode | Payload bytes | Count |
|---|---|---:|---:|
| RX | `0x4111` | 20 | 18 |
| TX | `0xA000` | 1 | 5 |
| RX | `0xA000` | 0 | 5 |
| TX | `0xA264` | 4 | 1 |
| RX | `0xA264` | 0 | 1 |
| RX | `0xA266` | 8 | 3 |

Each logged power write was followed by an empty same-opcode reply after
15–27 ms and a status push after 59–75 ms, measured from the log timestamps.
The timer write had an empty reply after 37 ms. The three timer reports
followed the write by 39 ms, 2.950 s and 60.247 s. The event log establishes
the shapes and order of these messages. Their payload values, sequence
numbers, result codes and measured wire timings are unavailable in that log.

All 144 samples (69 distinct payloads) matched the current status decoder.
Of these, 140 report power on, 138 report humidifying and 5 report water
shortage. Each payload starts with `38 00 01` (MCU 1.0.56). All show the tank installed, Manual
mode, the retained display setting and manual target at 100, warm level 0,
and zero at offsets 14–19. Reported humidity spans 36–74% and temperature
14–18 °C. The first sample reports mist level 5 and the other 143 report
level 9. These are device-reported readings and settings.

On the first observed water-shortage sample, offsets 5 and 7 changed to 1
and humidifying became false while power remained on. The first standby
sample arrived 10.044 s later with power off, the tank still installed,
water shortage still set, and fault byte 0. The retained mist and display
settings stayed at 9 and 100. The observations cover depletion with the
tank installed. Tank removal and refill are separate tests.

The event log and the status samples do not identify complete wire frames.
In the water-depletion file the source opcode of each sample is unknown.
Neither file preserves the original headers or checksums. The native test
replays four observed status payloads inside synthetic test envelopes.

### Bidirectional standby capture

On 26 November 2026 an assembled EU LUH-A602S-WEUR with MCU firmware 1.0.56
was recorded for 75 seconds at 9600 baud, 8N1. The ESP ran the temporary
ESPHome build `1.2.03-uart-trace`. Directions are relative to the ESP. The
34 consecutive chunks contain 63 complete frames (311 TX bytes, 640 RX bytes).
The lengths, checksums and recorded byte-stream hashes all validated. The
timestamps identify when ESPHome emitted each log chunk.

| Command | ESP-to-MCU frame | MCU-to-ESP frame | Count |
|---|---|---|---:|
| `0x4110` | `0x22`, empty request | `0x12`, 20-byte status | 15 pairs |
| `0xA265` | `0x22`, empty request | `0x12`, empty reply | 15 pairs |
| `0xA000` | `0x22`, one-byte `00` (OFF) | `0x12`, empty reply | 1 pair |
| `0x4111` | — | `0x02`, 20-byte unsolicited status | 1 report |

All frames used version 1 and result byte 0. Pairing by command and order
gave a reply for every request. Five `0x4110` replies echoed the request
sequence and ten used the next sequence value. All `0xA265` and `0xA000`
replies echoed their request sequence. The decoder accepts a valid status
report independently of sequence equality.

The empty `0xA265` replies carry no seconds value. Timer state remained
unknown during this run. The regression also checks that an empty reply
does not replace a known countdown. The explicit OFF command was sent
while the device was already in standby. The empty `0xA000` reply was
followed by a `0x4111` report confirming power off. The regression keeps
the command pending through the empty reply and confirms it on status.

All 16 status payloads reported an installed tank, water shortage, no
fault, power off and Manual mode. The retained display, mist and warm
settings were 100, 5 and 3. The display and derived humidifying and
heating states are off in this standby state.

The capture supplies wire-frame evidence for standby polling and OFF
confirmation with ESPHome on the ESP and original firmware on the MCU.
The local bundle contains both byte streams and the labelled decoded
frames. The public regression builds synthetic frames with the observed
headers, lengths and sequence relationships.

### Bidirectional tank and wet-operation capture

On 26 November 2026 the same closed EU unit ran `1.2.03-uart-trace` with
MCU 1.0.56. Unsolicited `0x4111` reports gave tank/shortage flags of 0/1
(empty tank installed), 1/0 (tank removed) and 0/0 (refilled tank installed).
A separate earlier run reported 1/1 with the tank removed. The tank flag
is authoritative for detecting removal. Both flags gate wet operation.

With water installed, physical power-on produced visible mist and lit the
display. The panel cycled mist through internal levels 1, 5 and 9 and warmth
through 0–3. The heater output was not measured. Home Assistant switched to
Humidity and changed the target to 60%. Selecting Sleep from Manual sent
`0x4082` with 55 in the payload and produced a status report with mode 2,
target 100 and a dark display. The earlier candidate timed out despite the
mode change. The panel Sleep button independently produced mode 2 with
target 100. Selecting Sleep from Humidity with a 60% target retained 60.
The native test replays the observed Manual-to-Sleep payloads and verifies
that the empty ACK alone cannot confirm the mode.

A Home Assistant one-minute timer sent `0xA264` with LE32 60. The MCU
reported remaining/total 60/60 and then 0/60 about 60.3 seconds later
with `0xA266`, and the unit switched off. The owner observed the mist stop.
These are interactions with ESPHome on the ESP, with the original MCU.
Stock ESP startup was outside the capture.

The owner then held the power button in standby. The UART received
`D102/02` for pairing and `D103/04` for factory reset 10 seconds later.
The owner's reset configuration rebooted the ESP and removed its
stored Wi-Fi connection. The encrypted network capture necessarily
paused while the ESP was off the configured network. Reprovisioning
reopened the encrypted API and closed the setup AP. Across the two capture
windows, 440 consecutive chunks (per connection) contained 724 records with
valid frame lengths and checksums. One record carried an unrecognised opcode
and nonzero result, so it was excluded from command interpretation.

## Observations from the assembled EU unit

| Check | Observation with the earlier local firmware |
|---|---|
| MCU version | Status bytes `38 00 01` decode to 1.0.56 |
| Tank absent | Payload bytes 4/5 were 1/1. An empty installed tank gave 0/1, and a filled tank gave 0/0 |
| Water runs out | Water shortage and inactive fog were reported while power was still on. The MCU entered standby about 10 seconds later without an ESP off command |
| Display | `0xA105` with 0/100 changed only the display during the test. Standby stayed dark with a retained value of 100. No independent button backlight control was demonstrated |
| Timer | A two-minute timer reported 120/120 and cancelled with 0/0. A one-minute timer reported 60/60, then 0/60 at expiry and switched the appliance off after about 60.5 seconds |
| Mist and warmth | The MCU confirmed mist 1–9 and warmth 0–3. The three app-scale selections map to 1, 5 and 9. The test did not measure heater output |

The earlier implementation also exercised time-based power on/off from its ESPHome
configuration. The firmware component leaves scheduling to the device config or
Home Assistant. Original Auto is a reported mode without a verified setter.

### Fault diagnostics

The EU stock ESP maps payload byte 15 to these service codes. The descriptions
name the fault category. They do not identify a failed part or measure heater
temperature.

| Raw | Display/category | Stock service code |
|---:|---|---:|
| 0 | No reported MCU fault | — |
| 1 | E1, dry-run protection | 11601000 |
| 2 | E2, PTC heater | 11602000 |
| 3 | E3, overtemperature | 11603000 |
| 4 | E4, water fault | 11604000 |
| 5 | Meaning unconfirmed for LV600S | 11014000 |
| 6 | E6, communication fault | 11607000 |

E6 and raw value 6 were observed together on the isolated bench setup. The
assembled unit reported 0 in normal operation. Other nonzero values are exposed
as raw codes. A disconnected or stale MCU reports unknown status.
