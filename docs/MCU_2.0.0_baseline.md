# Levoit Vital 200S Pro / MCU 2.0.0 — UART Protocol Baseline

## 1. Overview

What this document covers: the **observable wire behavior** of the
MCU on a Levoit Vital 200S Pro running MCU firmware **2.0.0** —
what it emits over UART on its own initiative, what it accepts from
the ESP side, and what its responses look like. This is the
canonical reference for the next contributor extending the ESPHome
component for this MCU revision.

Document scope contrast:

| Document                            | Scope                                                        |
|-------------------------------------|--------------------------------------------------------------|
| [`LEVOIT_UART.md`](../LEVOIT_UART.md) | Upstream protocol primer covering frame envelope and family-level CMD conventions across all Levoit models |
| `MCU_2.0.0_baseline.md` *(this doc)* | Wire-level behavior specific to MCU 2.0.0 on the Vital 200S Pro |
| [`STOCK_FIRMWARE_FINDINGS.md`](./STOCK_FIRMWARE_FINDINGS.md) | Stock-ESP-firmware disassembly: dispatcher, klv_pack, function entries, address references |

Cross-references in this document point to the other two; complete
overlap is avoided. Anything that requires reading the disassembled
firmware to verify lives in `STOCK_FIRMWARE_FINDINGS.md`; anything
that's a fact about MCU-emit or MCU-accept behavior lives here.

MCU firmware version is confirmed from the first status push: TLV
`0x01` carries `00 00 02` (patch / minor / major, little-endian),
i.e. **firmware 2.0.0**.

---

## 2. Status push (MCU → ESP)

### 2.1 Frame envelope and cadence

The MCU emits a single `CMD = 02 00 55` (SEND, `msg_type = 0x22`)
status frame every **~5 seconds** with no command needed. Every push
contains the **same 32 TLVs** in the same order. Frame length is
**114 bytes** (`<len>` byte `0x6C`).

The wire order has one quirk worth noting: TLV `0x20` is emitted
**before** TLV `0x1F`, consistent across every push observed.
TLV-id-ordered traversal is therefore unsafe; parsers must consume
TLVs in wire order rather than expecting ID-monotonic placement.

### 2.2 Complete TLV inventory

| TLV    | Len | Field                                            | Decoder in `vital_status.cpp` | HA entity                                  |
|--------|-----|--------------------------------------------------|-------------------------------|--------------------------------------------|
| `0x00` | 1   | Device ID byte                                   | logged only                   | —                                          |
| `0x01` | 3   | MCU version (patch / minor / major)              | publishes text_sensor         | `sensor.…_mcu_version`                     |
| `0x02` | 1   | Power                                            | drives `fan->apply_device_status(power=…)` | `fan.…`                       |
| `0x03` | 1   | Fan mode                                         | drives `fan->apply_device_status(mode=…)`  | `fan.…` preset                |
| `0x04` | 1   | Fan level                                        | drives `fan->apply_device_status(speed=…)` | `fan.…` speed pct             |
| `0x05` | 1   | Fan speed alt encoding                           | logged only                   | — (probably duplicate of `0x04`)           |
| `0x06` | 1   | Display illuminated                              | publishes switch              | `switch.…_display`                         |
| `0x07` | 1   | Display state                                    | logged only                   | —                                          |
| `0x08` | 1   | Unknown — always `0x00` observed                 | logged only                   | —                                          |
| `0x09` | 1   | AQI level (1–4)                                  | publishes sensor              | `sensor.…_aqi`                             |
| `0x0A` | 1   | Air quality detail (0=error, non-zero ok)        | publishes text_sensor         | `sensor.…_error`                           |
| `0x0B` | 2   | PM2.5 LE16                                       | publishes sensor              | `sensor.…_pm_2_5`                          |
| `0x0E` | 1   | Child lock                                       | publishes switch              | `switch.…_child_lock`                      |
| `0x0F` | 1   | Auto mode (Default / Quiet / Efficient)          | publishes select              | `select.…_auto_mode`                       |
| `0x10` | 2   | Efficiency room size raw LE16 (× 10.764 × 1.3 = m²) | publishes number          | `number.…_auto_mode_room_size`             |
| `0x11` | 2   | Efficiency high-fan seconds remaining LE16       | publishes text_sensor + sensor| `sensor.…_auto_high_remaining_run_time`    |
| `0x12` | 1   | Auto mode profile (sub-state)                    | logged only                   | —                                          |
| `0x13` | 1   | Light Detect toggle                              | publishes switch              | `switch.…_light_detect`                    |
| `0x16` | 1   | Wi-Fi LED state                                  | logged only                   | —                                          |
| `0x17` | 1   | Dark detected (ambient-light reading)            | publishes binary_sensor       | `binary_sensor.…_dark_detected`            |
| `0x18` | 1   | Sleep type                                       | publishes select + caches     | `select.…_sleep_mode_type`                 |
| `0x19` | 1   | Quick-clean enabled                              | publishes switch + caches     | `switch.…_quick_clean_preset`              |
| `0x1A` | 2   | Quick-clean minutes LE16                         | publishes number + caches     | `number.…_quick_clean_minutes`             |
| `0x1B` | 1   | Quick-clean fan                                  | publishes number + caches     | `number.…_quick_clean_fan_level`           |
| `0x1C` | 1   | White-noise enabled                              | caches (no entity — see §7)   | —                                          |
| `0x1D` | 2   | White-noise minutes LE16                         | caches (no entity)            | —                                          |
| `0x1E` | 1   | White-noise fan                                  | caches (no entity)            | —                                          |
| `0x1F` | 1   | Sleep fan                                        | publishes number + caches     | `number.…_sleep_fan_level_5_auto`          |
| `0x20` | 2   | Sleep minutes LE16                               | publishes number + caches     | `number.…_sleep_mode_minutes`              |
| `0x21` | 1   | Daytime enabled                                  | publishes switch + caches     | `switch.…_daytime_preset`                  |
| `0x22` | 1   | Daytime mode                                     | publishes select + caches     | `select.…_daytime_fan_mode`                |
| `0x23` | 1   | Daytime fan level                                | publishes number + caches     | `number.…_daytime_fan_level`               |

"Caches" in the decoder column refers to `bulk_prefs_` (declared in
`components/levoit/levoit.h`), the cache that the bulk-preferences
SET builder reads to fill non-edited fields — see §4 below.

### 2.3 TLVs not emitted by this model

`0x0C`, `0x0D` (Sprout-only PM1.0 / PM10), `0x14`, `0x15`
(unallocated / unused in this firmware), `0x24`, `0x25`, `0x26`,
`0x27` (Sprout-only LED ring / breathing / fan-RPM). All eight have
decoder cases in `vital_status.cpp` but they're gated by
`model == SPROUT` so they never run on a Vital.

### 2.4 Discrepancies vs `LEVOIT_UART.md`

The upstream protocol primer documents fixed lengths for several
TLVs that differ from what the wire actually emits on MCU 2.0.0:

| TLV   | LEVOIT_UART.md says | Wire is | Notes                                              |
|-------|---------------------|---------|----------------------------------------------------|
| `0x00`| 4 bytes             | 1 byte  | Device ID on Vital 200S Pro is a single byte (`0x02`) |
| `0x1A`| 1 byte              | 2 bytes | Quick-clean minutes is LE16                        |
| `0x1D`| 1 byte              | 2 bytes | White-noise minutes is LE16                        |
| `0x20`| 1 byte              | 2 bytes | Sleep-mode minutes is LE16                         |

The existing parser uses the **wire length byte** (`len_code`)
rather than spec-fixed lengths, so all four decode correctly today;
`LEVOIT_UART.md` could be updated to reflect the actual lengths for
this MCU revision.

---

## 3. ESP → MCU commands

### 3.1 Frame envelope

ESP→MCU frames use the standard envelope described in
[`LEVOIT_UART.md`](../LEVOIT_UART.md) and broken down at the
byte level in [`STOCK_FIRMWARE_FINDINGS.md` §2.1](./STOCK_FIRMWARE_FINDINGS.md#21-frame-envelope):

```
A5 22 <ctr> <len> 00 <chk> <cmd[0]> <cmd[1]> <cmd[2]> 00 <payload...>
```

The `<ctr>` byte increments per outgoing packet (independent counter
from the MCU→ESP direction). `<chk>` is computed across the whole
frame excluding the checksum byte itself.

### 3.2 SET-payload tag namespace is local to each CMD byte

Cardinal fact for anyone designing a new SET command:

**Status-push TLV tag IDs (`0x00`–`0x27`) are NOT the same as
SET-command payload tag IDs.** Each `02 <subsys> 55` CMD has its
own local payload-tag namespace that conventionally starts at
`0x01`. A SET frame that re-uses status-TLV IDs against the wrong
CMD byte is well-formed but nonsensical to the MCU: the frame gets
the standard `0x12` ACK (received and parsed) and the unrecognised
tags are silently dropped.

Confirmed local-tag → status-TLV correspondences:

| ESP→MCU command (CMD bytes)         | Local payload tags                                     | Status TLV(s) updated |
|-------------------------------------|--------------------------------------------------------|------------------------|
| `02 02 55` set fan mode             | `01` = fan mode value (`0x00/0x01/0x02/0x05`)          | `0x03`                 |
| `02 02 55` set auto-mode pref       | `02` = auto-mode index, `03` = room size LE16           | `0x0F`, `0x10`         |
| `02 02 55` bulk-preferences SET     | `04`..`0F` (12 TLVs — see §4)                          | `0x18`..`0x23`         |
| `02 03 55` set fan speed            | `01` = fan speed (1–4)                                 | `0x04`                 |
| `02 04 55` set display              | `01` = display brightness                              | `0x06` / `0x07`        |
| `02 11 55` set light detect         | `01` = on/off                                          | `0x13`                 |
| `02 40 51` set child lock           | `01` = on/off                                          | `0x0E`                 |
| `02 18 50` set Wi-Fi LED            | `01`..`04` (mode + colors)                             | `0x16`                 |
| `02 19 50` set timer                | `01` = duration in seconds (4-byte LE)                 | — (timer ack via `02 1A 50`) |

The dispatcher-level evidence for how the MCU routes a `02 XX 55`
CMD byte to its handler lives in
[`STOCK_FIRMWARE_FINDINGS.md` §3](./STOCK_FIRMWARE_FINDINGS.md#3-the-uart-send-dispatcher-0x42006e46).

### 3.3 Confirmed command shapes — observed scenarios

The following ESP→MCU frames have been captured on the wire and the
resulting status-push TLV changes observed. These shapes are
patterns to mirror when designing additional SET commands.

#### `fan.set_preset_mode preset_mode=Sleep`

```
A5 22 4D 07 00 88 02 02 55 00 01 01 01
                  └ CMD ┘    └ TLV: tag=01 len=01 val=01 (Sleep) ┘
```
Code: `CommandType::setFanModeSleep`. TLV changes:
- `0x03` 2 → 1 (fan mode → Sleep)
- `0x06` 1 → 0 (Sleep mode dims the display as a side effect)

The preference TLVs `0x18` / `0x1F` / `0x20` do **not** change —
entering Sleep mode via the preset switches the active fan mode
(TLV `0x03`), not the sleep *preference* values.

#### `fan.set_preset_mode preset_mode=Pet`

```
A5 22 52 07 00 7F 02 02 55 00 01 01 05
                  └ CMD ┘    └ TLV: tag=01 len=01 val=05 (Pet) ┘
```
Code: `CommandType::setFanModePet`. TLV changes:
- `0x03` 2 → 5 (fan mode → Pet)
- `0x04` 0 → 3 (Pet uses fan level 3 by default)

#### `fan.set_preset_mode preset_mode=Auto`

```
A5 22 56 07 00 7E 02 02 55 00 01 01 02
                  └ CMD ┘    └ TLV: tag=01 len=01 val=02 (Auto) ┘
```
Code: `CommandType::setFanModeAuto`. TLV changes:
- `0x04` 0 → 4 (Auto kicked fan up to level 4 in response to room)
- `0x11` 0 → 0x155 (efficiency high-fan counter started counting)

#### `select.select_option Quiet` on `auto_mode`

```
A5 22 64 0B 00 67 02 02 55 00 02 01 01 03 02 00 00
                  └ CMD ┘    └ TLV 02 = Auto Quiet ┘ └ TLV 03 = room size 0 ┘
```
Code: `CommandType::setAutoModeQuiet`. TLV changes:
- `0x0F` 2 → 1 (auto mode profile → Quiet)

#### `select.select_option "Room Size"` on `auto_mode`

```
A5 22 68 0B 00 03 02 02 55 00 02 01 02 03 02 5E 01
                  └ CMD ┘    └ TLV 02=Efficient ┘ └ TLV 03=size raw 0x015E ┘
```
Code: `CommandType::setAutoModeEfficient` (with `efficiency_room_size`
number state ≈ 25 m² → raw 0x015E = 350). TLV changes:
- `0x04` 0 → 4 (fan ramped up in response to room-size mode)
- `0x11` 0 → 0x156 (counter incrementing)

#### `switch.toggle` on display

```
A5 22 76 07 00 5E 02 04 55 00 01 01 00
                  └ CMD ┘    └ TLV: tag=01 len=01 val=00 (off) ┘
```
Code: `CommandType::setDisplayOff`. CMD bytes differ from the fan-mode
family: `02 04 55` instead of `02 02 55`. TLV changes:
- `0x06` 1 → 0 (display illuminated → off)
- `0x07` 1 → 0 (display state → off)

#### `switch.toggle` on light_detect

```
A5 22 82 07 00 45 02 11 55 00 01 01 00
                  └ CMD ┘    └ TLV: tag=01 len=01 val=00 (off) ┘
```
Code: `CommandType::setLightDetectOff`. CMD bytes: `02 11 55`. TLV
changes:
- `0x13` 1 → 0 (light detect → off)
- `0x12` 1 → 0 (auto-mode profile reset — light_detect interacts with auto)

### 3.4 Subsystem-byte mapping

Compact reference for the `<cmd[1]>` byte under the `02 XX 55`
family, with the status TLVs each subsystem affects:

| `<cmd[1]>` | Subsystem                                    | Status TLV(s) reflected |
|------------|----------------------------------------------|-------------------------|
| `0x02`     | Fan mode + auto-mode prefs + bulk-prefs SET  | `0x03`, `0x0F`, `0x10`, `0x18`..`0x23` |
| `0x03`     | Fan speed (manual)                           | `0x04`                  |
| `0x04`     | Display                                      | `0x06`, `0x07`          |
| `0x11`     | Light detect                                 | `0x13`                  |
| `0x40` *(02 40 51 family)* | Child lock                     | `0x0E`                  |
| `0x18` *(02 18 50 family)* | Wi-Fi LED                      | `0x16`                  |
| `0x19` *(02 19 50 family)* | Timer                          | — (separate ack frame)  |

---

## 4. Bulk-preferences SET (CMD 02 02 55 tags 0x04–0x0F)

### 4.1 Overview

The MCU's sleep / quick-clean / white-noise / daytime preference
subsystem is **not** a per-CMD-byte family — it is a single
**12-TLV bulk write** under the **same `02 02 55` CMD byte** that
also handles fan-mode and auto-mode preferences. The handler
dispatches on tag IDs within the payload:

| Tag range in `02 02 55` payload | Handler                                                   |
|--------------------------------|-----------------------------------------------------------|
| `0x01`                         | Fan mode (existing `setFanMode*`)                         |
| `0x02` + `0x03`                | Auto-mode preference (existing `setAutoMode*`)            |
| `0x04`..`0x0F`                 | Bulk preferences (sleep / QC / WN / daytime)              |

The three subsystems share the CMD byte because their local-tag
namespaces are disjoint. For the function-level disassembly of the
bulk-prefs writer (entry `0x420075a6`, 12 chained `klv_pack` calls,
total wire frame 49 bytes), see
[`STOCK_FIRMWARE_FINDINGS.md` §4](./STOCK_FIRMWARE_FINDINGS.md#4-bulk-preferences-set--the-12-tlv-write).

### 4.2 SET-tag → status-TLV mapping

**Monotonic rule: `set_tag = status_tlv − 0x14`**, with field
lengths matching exactly.

| SET tag | Length | → Status TLV | Field                  |
|---------|--------|--------------|------------------------|
| `0x04`  | 1 B    | `0x18`       | sleep_type (gate — §4.3) |
| `0x05`  | 1 B    | `0x19`       | quick_clean_enabled    |
| `0x06`  | LE16   | `0x1A`       | quick_clean_minutes    |
| `0x07`  | 1 B    | `0x1B`       | quick_clean_fan        |
| `0x08`  | 1 B    | `0x1C`       | white_noise_enabled    |
| `0x09`  | LE16   | `0x1D`       | white_noise_minutes    |
| `0x0A`  | 1 B    | `0x1E`       | white_noise_fan        |
| `0x0B`  | 1 B    | `0x1F`       | sleep_fan              |
| `0x0C`  | LE16   | `0x20`       | sleep_minutes          |
| `0x0D`  | 1 B    | `0x21`       | daytime_enabled        |
| `0x0E`  | 1 B    | `0x22`       | daytime_mode           |
| `0x0F`  | 1 B    | `0x23`       | daytime_level          |

The mapping is interleaved across clusters (sleep type at `0x04`,
quick-clean fields at `0x05`–`0x07`, white-noise fields at `0x08`–`0x0A`,
sleep fan/min at `0x0B`–`0x0C`, daytime fields at `0x0D`–`0x0F`),
matching the order of `klv_pack` calls in the stock-firmware
writer — see [`STOCK_FIRMWARE_FINDINGS.md` §5](./STOCK_FIRMWARE_FINDINGS.md#5-set-tag--status-tlv-mapping)
for the full disassembly-derived mapping.

### 4.3 Tag `0x04` gate behavior

Empirically verified by on-device writes against MCU FW 2.0.0:

- When the bulk-write frame contains `tag=0x04 value=0x00`
  (sleep_type = Default), the MCU applies **only** the `0x04`
  change to status TLV `0x18`. Writes to tags `0x05..0x0F` are
  silently dropped: the MCU returns the standard `0x12` ACK and
  the subsequent status push shows the original values for the
  other 11 fields.
- When `tag=0x04 value != 0x00` (observed non-zero values are
  `0x01` and `0x02`, corresponding to "Custom1" / "Custom2" in the
  stock app), the MCU applies all 12 writes.

Storage model: a **single shared field set**, not per-preset slots.
Switching the type byte does not switch among different stored
field values; the cluster TLVs always reflect the most recent
non-Default write. See
[`STOCK_FIRMWARE_FINDINGS.md` §4.5](./STOCK_FIRMWARE_FINDINGS.md#45-storage-model)
for the experimental evidence.

### 4.4 Implementation pattern for ESPHome components

When editing any sleep / QC / WN / daytime field while the active
type is Default, the gate semantics in §4.3 require one of two
design choices for client UX:

**Option A — auto-bump on edit.** Send the bulk write with
tag `0x04` temporarily set to a non-Default value (e.g. `0x01`),
then send a second bulk write with `0x04 = 0x00` to restore the
type label. Two frames per user edit; the type byte flickers
visibly on the wire and in the status push during the
transaction.

**Option B — surface the constraint via conditional availability.**
Expose the type as a writable select and document that other
cluster edits only persist while type ≠ Default. The user changes
the type before adjusting other fields. One frame per edit; the
type byte stays stable; the constraint is visible in the UX.

This component chose **Option B**. Rationale: one wire frame per
edit (no transient type-byte flicker visible in HA or in the MCU
status push), no rollback story to design if the second frame in
an auto-bump pair fails, and the constraint matches the stock
VeSync app's UX behavior (its Sleep Preset UI requires selecting a
Custom slot before allowing other fields to be edited). Option A
remains reasonable for future implementations whose constraints
favor automation-driven edits where the type-byte flicker is
acceptable.

Either approach reads the other 11 fields from a cache populated
by the status decoder (`bulk_prefs_` in `components/levoit/levoit.h`,
updated via `Levoit::update_bulk_pref()` from the `0x18`..`0x23`
decoder cases in `vital_status.cpp`). The cache is filled by the
first decode of a status push at boot — see §6.

---

## 5. Unmapped or partially-mapped CMD bytes

### 5.1 Dispatcher-key inventory and suspected purposes

The stock firmware's UART dispatcher accepts a wider set of
`02 XX 55` / `02 XX 51` / `02 XX 50` keys than the ESPHome component
currently uses. The following are observed in the dispatcher map
but have no implementation in `vital_commands.cpp`:

| CMD bytes    | Stock-fw payload shape                                                              | Suspected feature                                  |
|--------------|-------------------------------------------------------------------------------------|----------------------------------------------------|
| `02 01 50`   | empty payload                                                                       | unknown (single dispatcher reference)              |
| `02 04 50`   | various                                                                             | timer-related (4 references)                       |
| `02 04 51`   | `{01 01 byte}` (single value)                                                       | possible filter status indicator                   |
| `02 05 51`   | `{01 01 a, 02 01 b}` (two bytes)                                                    | possible filter calibration                        |
| `02 07 51`   | empty payload (query)                                                               | probable read-back paired with `02 04 51` / `02 05 51` |
| `02 41 51`   | `{01 01 byte}`                                                                      | child-lock variant                                 |
| `02 44 51`   | unknown call site                                                                   | unknown                                            |
| `02 01 55`   | empty payload (single site)                                                         | unknown — looks like a "request status" trigger    |
| `02 05 55`   | site A: `{01 01 byte}`; site B: `{03 00}` empty-value query                         | orthogonal to preferences — see §5.2               |
| `02 0A 55`   | single site                                                                         | Sprout-family overlap (white-noise on Sprout) — unused on Vital |

Function-entry PCs and call-site PCs for each key are catalogued in
[`STOCK_FIRMWARE_FINDINGS.md` §3.3](./STOCK_FIRMWARE_FINDINGS.md#33-identified-key--cmd-mappings)
and [`§8.2`](./STOCK_FIRMWARE_FINDINGS.md#82-dispatcher-call-sites-by-key).

### 5.2 CMD `02 05 55` — orthogonal to preferences

This CMD was investigated as a sleep-preferences SET candidate
based on dispatcher-multiplicity heuristics, then ruled out: frames
matching the stock-firmware's site-A shape (`A5 22 <ctr> 07 00 <chk>
02 05 55 00 01 01 <byte>`) sent on-device produce a clean `0x12` ACK
but no change to any preference TLV. The actual sleep / QC / WN / DT
write path is the bulk-prefs SET at CMD `02 02 55` (§4).

Circumstantial disassembly evidence (counter at DRAM `0x3fc9d7a4`
incremented before each call, 20-byte struct memcpy in the same
basic block, surrounding compilation-unit string constants like
`filter dust percent` and `filter use time`) points to a
filter-monitor report channel. Full evidence is in
[`STOCK_FIRMWARE_FINDINGS.md` §6](./STOCK_FIRMWARE_FINDINGS.md#6-cmd-02-05-55-key-0x5505--analyzed-orthogonal-to-preferences),
including the explicit "Hypothesis (based on call-site context,
unverified)" qualification.

Confirming or refuting the filter-monitor hypothesis would
require capturing this CMD's emission during a stock-firmware
filter-life event — see §7 gap #5.

### 5.3 Stock-firmware filter functionality not yet covered

The stock ESP firmware exposes several filter-related computations
that the ESPHome component does not currently surface. Evidence is
the DROM string pool of the stock image:

| Stock-fw feature                 | String evidence                                                                          | Current component coverage                                                                            |
|----------------------------------|------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------|
| Filter dust calculation          | `filter dust percent: %d`, `filter dust = %d`                                            | Not surfaced. Component computes `filter_life_left` locally from CADR × time, not from MCU readings. |
| Filter usage hours               | `filter use time = %d`, `filter use Time: %d`                                            | Not surfaced.                                                                                          |
| Filter min/max lifetime hours    | `filter max liftetime hour`, `filter min liftetime hour` *(typo from stock binary)*       | Not surfaced. Component exposes `filter_lifetime_months` as user input only.                          |
| Filter algorithm version         | `filter algorithm version = 0x%04x`                                                      | Not surfaced.                                                                                          |
| Cleaning-before-bed feature      | `cleaningBeforeBe[d]`                                                                    | Not surfaced.                                                                                          |
| Pet mode (`enterPetMode`)        | symbol present in stock binary                                                           | Covered as `fan.…` preset "Pet"                                                                       |

Filter monitoring appears to live behind the `02 05 55` channel
(§5.2). The status push does not carry filter-hours or
dust-percent data, so reading the MCU's actual filter measurements
would require a query/response transaction on a separate CMD,
not in the current status-push parser.

---

## 6. ESPHome × MCU interaction — boot-time FIFO retention decode

A subtle behavior worth documenting in advance, since reading the
device's API-streamed logs without this context naturally suggests
the dedup is broken. **This is normal operation, not a bug.**

### 6.1 The mechanism

- The MCU pushes a `CMD = 02 00 55` status frame every 5 s on its
  own clock, independent of any ESP state.
- ESP32-C3 UART RX has a **128-byte hardware FIFO** that begins
  accumulating received bytes from the moment the GPIO pin is
  configured — well before any application code runs.
- A status frame is 114 bytes, which fits cleanly in one FIFO load.
- After an ESP OTA reboot, the bootloader + IDF startup + ESPHome
  `App::setup()` takes ~3–5 s before the API server is ready to
  accept client connections.

### 6.2 What happens at boot

If an MCU status push arrives during the boot window (which it
typically does — every 5 s of boot, statistically guaranteed):

1. Bytes land in the hardware UART FIFO. No driver listening yet.
2. ESPHome's UART driver initialises during component setup. FIFO
   contents are drained into the software RX buffer.
3. The levoit frame parser reads the buffered bytes, recognises a
   complete `A5 22 ...` frame, calls `process_message` →
   `dispatch_decoder` → `payload_changed_` (which sees an empty
   cache, claims `cache[0]`, returns `true`) → `decode_vital_status`.
4. `decode_vital_status` iterates all 32 TLVs and calls the
   corresponding `publish_*` methods on each entity. Each entity
   ends with `has_state() == true` and the correct value, even
   though no API client is connected yet.
5. ESPHome's task log buffer (default **768 bytes**) records the
   `dispatch:` and `dedup:` log lines. The boot dump from
   `App::setup()` then writes ~10 KB of `dump_config` output,
   overwriting that log line many times over before the API
   client finally connects.
6. The API client connects and sends `SubscribeStatesRequest`.
   ESPHome's `InitialStateIterator` (in
   `components/api/subscribe_state.{h,cpp}`) walks every entity
   and calls `send_<type>_state` for each, which transmits `state`
   + `missing_state = !has_state()`. HA receives the correct state
   for every entity that successfully published.
7. The **next** MCU push arrives ~5 s later with identical
   payload bytes (no state has changed). Hash matches `cache[0]`,
   `payload_changed_` returns `false`, decoder logs
   `skip decode (unchanged payload)`. This is correct — repeated
   decodes would be no-ops anyway, since the publish methods
   dedup on value equality internally.

### 6.3 Why this looks alarming in logs

From the API-streamed log alone, every visible `0055` push is
`skip decode`, and no visible `dispatch:` for any decode that ever
ran. The dedup appears broken ("never decodes anything"). It isn't
— the one successful decode happened pre-handshake, its log line
was eaten by the 768-byte ring buffer, and the dedup is correctly
short-circuiting visually-redundant subsequent pushes.

### 6.4 How to confirm

The decisive test is to dump the cache contents at the top of
`payload_changed_`:

```cpp
ESP_LOGI("levoit.dedup",
         "ENTER model=%u ptype=%02X%02X computed_hash=0x%02X",
         model, ptype0, ptype1, hash);
for (uint8_t i = 0; i < 16; i++) {
  if (g_payload_hash_cache[i].valid)
    ESP_LOGI("levoit.dedup",
             "  cache[%u]: model=%u ptype=%02X%02X hash=0x%02X",
             i, g_payload_hash_cache[i].model,
             g_payload_hash_cache[i].ptype0,
             g_payload_hash_cache[i].ptype1,
             g_payload_hash_cache[i].hash);
}
```

On the first visible `payload_changed_` call for a given `ptype`,
the cache will already contain a matching entry. Comparing the
`seq` byte in the visible push to the one that would have arrived
5 s earlier shows the gap matching the boot / UART-init window.

### 6.5 Implication for new features

Entities only need to publish correctly **on first decode** to
reach HA — the `InitialStateIterator` handles the rest. If an
entity shows `unknown` in HA despite the MCU pushing a valid
value, the cause is **not** the dedup; check whether the
component's `publish_<entity>` rejected the value (e.g.
`Levoit::publish_select` early-returns when
`value >= options.size()`, leaving `has_state()` at false). The
fix is on the entity-config side, not the decoder side.

The `bulk_prefs_` cache populates from this same boot-time
FIFO-retention decode. By the time the API client connects, the
cache is already valid, so `setBulkPrefs` can fire immediately
when the user toggles an entity. No "wait for first decode"
gating is required.

---

## 7. Gap analysis — MCU functionality not yet covered by ESPHome entities

Ranked by user-visible utility (high → low). "Status" reflects the
state of each gap as of this revision.

| # | Gap                                                                          | Source                                                                              | Suggested entity                                                                                 | Effort         | Utility       | Status                                                                                                                         |
|---|------------------------------------------------------------------------------|-------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------|----------------|---------------|--------------------------------------------------------------------------------------------------------------------------------|
| 1 | Sleep / QC / DT cluster SET — writable cluster fields                        | CMD `02 02 55` tags `0x04..0x0F` (§4)                                               | Writable entities for sleep type / fan / min, QC enabled / fan / min, DT enabled / mode / level | 1–2 h          | High          | **CLOSED.** 9 writable entities, all routing through `setBulkPrefs`. See `LevoitSwitch` / `LevoitNumber` / `LevoitSelect` configs in `devices/levoit-vital200s/`. |
| 1a | Bulk-prefs SET — WN cluster (TLVs `0x1C`/`0x1D`/`0x1E`)                     | CMD `02 02 55` tags `0x08`/`0x09`/`0x0A`                                            | Writable entities for wn_enabled / wn_min / wn_fan                                               | 15 min         | None for Vital 200S Pro | **NOT IMPLEMENTED.** Vital 200S Pro has no white-noise hardware. The cluster is cache-only on this model and echoed back unchanged in every `setBulkPrefs` write (the MCU still requires all 12 TLVs in the bulk frame, per §4). Future Sprout work would add these entities and reuse the same SET path. |
| 2 | Auto-mode profile (TLV `0x12`)                                               | `0x12` status push                                                                  | sensor or text_sensor; meaning unclear (likely a sub-state of `0x0F`)                            | 15 min         | Low–Medium    | OPEN — currently logged only.                                                                                                  |
| 3 | Wi-Fi LED state read-back (TLV `0x16`)                                       | `0x16` status push                                                                  | switch or sensor; component sets the LED via `setWifiLed*` but never reads it                    | 15 min         | Low           | OPEN — currently logged only.                                                                                                  |
| 4 | Display state vs Display illuminated (TLV `0x07` vs `0x06`)                  | `0x07` status push                                                                  | possibly a `_display_state` sensor distinguishing "off / dim / full / forced-off-by-light-detect" | 15 min         | Low           | OPEN — `0x07` currently logged only.                                                                                           |
| 5 | Filter actual usage hours / dust % — MCU-side measurement                    | Suspected CMD `02 05 55` filter-monitor channel (§5.2)                              | sensor exposing MCU-reported filter use vs the component's CADR-based estimate                   | 2–4 h          | Medium        | OPEN — would replace the heuristic with the real number. Requires a new query / response handler.                              |
| 6 | Device ID (TLV `0x00`)                                                       | `0x00` status push                                                                  | none needed; ESPHome handles `unique_id` differently                                              | 0              | None          | NOT APPLICABLE — gap reframed as out-of-scope rather than open.                                                                |
| 7 | Filter calibration / algorithm version                                       | DROM strings only; no observed TLV (§5.3)                                           | —                                                                                                | unknown        | Low           | OPEN — diagnostic value only; not in any status push or known CMD.                                                             |
| 8 | Pet mode / Sleep / Auto / Manual as writable select                          | Already via `fan.set_preset_mode`                                                   | —                                                                                                | 0              | covered       | COVERED — no change.                                                                                                           |

The single high-priority gap (#1, bulk-prefs SET) is now CLOSED.
Remaining open gaps (#2, #3, #4, #5, #7) are incremental polish or
require new MCU channels not yet identified. Gap #1a (WN cluster)
is a Sprout-tracked item, not a Vital 200S Pro gap.

---

## 8. Appendix: Restore-to-baseline values for bulk-pref TLVs

The following sequence restores all 12 bulk-pref TLVs to clean
defaults. Order matters: the gate-close write (step 9) is
intentional, since closing the gate first would lock the subsequent
value writes. Pace each write with ~3–5 seconds between commands so
the MCU status push reflects each change before the next command
fires.

**Pre-check**: confirm `select.…_sleep_mode_type` is `Custom1` or
`Custom2` (gate open) before starting. If not, set it to `Custom1`
first — this is not part of the numbered sequence below.

| # | Entity                                | Target value  | TLV    | SET tag |
|---|---------------------------------------|---------------|--------|---------|
| 1 | `switch.…_quick_clean_preset`         | `off` (0)     | `0x19` | `0x05`  |
| 2 | `number.…_quick_clean_minutes`        | `5`           | `0x1A` | `0x06`  |
| 3 | `number.…_quick_clean_fan_level`      | `3`           | `0x1B` | `0x07`  |
| 4 | `switch.…_daytime_preset`             | `off` (0)     | `0x21` | `0x0D`  |
| 5 | `select.…_daytime_fan_mode`           | `Auto` (2)    | `0x22` | `0x0E`  |
| 6 | `number.…_daytime_fan_level`          | `3`           | `0x23` | `0x0F`  |
| 7 | `number.…_sleep_mode_minutes`         | `480`         | `0x20` | `0x0C`  |
| 8 | `number.…_sleep_fan_level_5_auto`     | `5`           | `0x1F` | `0x0B`  |
| 9 | `select.…_sleep_mode_type`            | `Default` (0) | `0x18` | `0x04`  |

Step 9 closes the gate. By §4.3 the gate-close write flips only TLV
`0x18` and leaves TLVs `0x19..0x23` at the values set by steps 1–8.

The three white-noise TLVs (`0x1C` / `0x1D` / `0x1E`) are
**preserved at the MCU's existing values** by every bulk write —
they have no writable entity on this model, and the SET payload
echoes the cache value back. There is no need to set them in a
restore-to-baseline sequence.

---

## Footer

| Item                            | Value                                              |
|---------------------------------|----------------------------------------------------|
| Device                          | Levoit Vital 200S Pro (LAP-V201S-AEUR)             |
| MCU firmware version            | 2.0.0 (confirmed via TLV `0x01` = `00 00 02`)      |
| Initial capture date            | 2026-05-19                                         |
| Final verification date         | 2026-05-20                                         |
| ESPHome component branch        | `feature/vital200s-extended`                       |

Cross-references:

- [`../LEVOIT_UART.md`](../LEVOIT_UART.md) — upstream protocol primer (frame envelope, model families)
- [`./STOCK_FIRMWARE_FINDINGS.md`](./STOCK_FIRMWARE_FINDINGS.md) — stock-firmware disassembly (dispatcher, klv_pack, function entries, address references)
