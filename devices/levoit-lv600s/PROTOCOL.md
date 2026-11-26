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
| 8 | Target RH. Manual mode reported 100, which is outside the adjustable 40–80% range |
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
The committed fixtures are synthetic. The initial component ran on the
assembled EU unit through the encrypted owner firmware. That run confirmed
power, display, mist, warmth, modes, target humidity, timer start and cancel.
The newer queue safety correction passed synthetic fault and recovery tests
and the real empty-tank check. Filled-tank and physical reset checks of this
build remain open. [Release review](RELEASE-REVIEW.md)

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
