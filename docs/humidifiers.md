[← Back to the project README](../README.md)

# Levoit humidifiers on the `levoit` component: plan

Three Levoit humidifiers have been opened, probed and decoded so far, each by a
different person, each in a different shape: one is a `model:` on the
component, one is a separate component in an open PR, one is a standalone
YAML in an issue. This page is the plan for bringing all of them, and the next
ones, onto the [`levoit`](../components/levoit/README.md) component as plain
`model:` entries with one shared set of entities, one device-folder layout and
one chapter in [LEVOIT_UART.md](../LEVOIT_UART.md).

The humidifiers speak the same frame as the purifiers, so the component already
fits. What they add is a second baud rate, two more status layouts, and a
handful of entities the purifiers do not have.

## Where each device stands

| Device | Protocol | Status | Who | Where |
|--------|----------|--------|-----|-------|
| **Superior 6000S** (LEH-S601S / S602S) | TLV, 115200, `cmd[0] = 02`, status `02 30 55` | `model: SUPERIOR6000S` since #64; runs on the stock module on two units | decode by Jyers, pins and tests by egoleos, jasongill, rebpdx | [issue #10](https://github.com/tuct/levoit/issues/10), [discussion #78](https://github.com/tuct/levoit/discussions/78), [`devices/levoit-superior-6000s`](../devices/levoit-superior-6000s/) |
| **LV600S** (LUH-A602S-WEUR, EU) | fixed layout, **9600**, `cmd[0] = 01`, status `01 10 41` / `01 11 41`, 20 bytes | open [PR #152](https://github.com/tuct/levoit/pull/152): separate `lv600s_humidifier` component, tested on one EU unit with MCU 1.0.56 | mvs90 | PR #152, its `PROTOCOL.md` and `HARDWARE.md` |
| **Dual 200S** (LUH-D302S-WDE) | fixed layout, **9600**, `cmd[0] = 01`, status `01 C1 40`, 25 bytes | standalone single-file YAML (MIT) with an inline parser, works on one unit with a XIAO ESP32-C3 added | gorischek | [issue #66](https://github.com/tuct/levoit/issues/66) |
| OasisMist LV450S / LV600S (US) | unknown | FCC filings only, nobody has opened one | — | [adding-a-new-device.md](./adding-a-new-device.md) |

All three opened units carry an ESP32-C3-SOLO-1 (the LV600S an ESP32-SOLO-1C)
next to a separate MCU, with the MCU link on the module's IO18 / IO19 pads, the
same as the Superior 6000S write-up. So the two install paths the purifiers
have apply: flash the stock module, or park it (`EN` to GND) and add a XIAO.

## The protocols side by side

Frame, counter and checksum are the ones in
[LEVOIT_UART.md](../LEVOIT_UART.md#frame-format) for every model. The
Superior is a TLV device like the Vital; the two LUH models are fixed-field
devices like the Core purifiers, and the two share most of their command set.

### Fixed-layout humidifiers (LV600S, Dual 200S) against the Core purifiers

| Function | Core purifiers | LV600S (#152) | Dual 200S (#66) |
|----------|----------------|---------------|-----------------|
| Baud | 115200 | 9600 | 9600 |
| Status | pushed, `01 30/B0/60 40`, `01 40 41` | polled with `01 10 41`, pushed as `01 11 41`; 16 or 20 bytes | polled with `01 C0 40`, reply `01 C1 40` in the `0x12` ACK; 25 bytes |
| Power | `01 00 A0` `{00/01}` | same | same |
| Display | `01 05 A1` `{00/64}` | same | same |
| Wi-Fi LED | `01 29 A1` `{state, on_ms LE16, off_ms LE16, reps}` | same | same |
| Mist / fan level | `01 60 A2` `{01, 01, speed}` | `01 60 A2` `{warm_en, mist_en, level}`, level 1..9 | `01 60 A2` `{00, 01, level}`, level 1..2 |
| Mist level, direct | — | `01 13 41` `{level, 00, 00, 00}` | — |
| Warm mist | — | `01 12 41` `{0..3, 00 ×4}` | not on this model |
| Humidity target | — | `01 E8 A2` `{00, target}`; `01 14 41` `{target, 00 ×3}` (Humidity mode); `01 82 40` `{target, 00 ×5}` (Sleep) | `01 80 40` `{t, t-5, t+5, 02, 01, 01}`, switches to Auto |
| Auto-off at target | — | — | `01 E5 A5` `{00/01}` |
| Timer | `01 64/65/66 A2` | same | not seen yet |
| Power-button hold | `01 02 D1` | same, plus `01 03 D1` reset | not seen yet |

Status layouts, payload offsets:

| Field | LV600S (20 bytes) | Dual 200S (25 bytes) |
|-------|-------------------|----------------------|
| MCU version patch, minor, major | 0..2 | 0..2 |
| power | 3 | 3 |
| tank removed | 4 | 4 |
| water shortage | 5 | unknown, suspected in 14..24 |
| display setting (0/100) | 6 | unknown |
| fog / humidifying | 7 | unknown |
| target % | 8 | 9 |
| current % | 9 | 10 |
| temperature °C | 10 | 11 |
| mode | 11: 0 Auto (read-only), 1 Manual, 2 Sleep, 3 Humidity | 12: 0 Auto, 1 Manual |
| mist level | 12: 1..9 | 13: 0 off, 1 low, 2 high |
| warm level | 13: 0..3 | — |
| fault code | 15 | unknown |

Same family, two layouts, so each decode is guarded by model, as the Core 300S
already is. Everything in the Dual 200S column comes from gorischek's YAML; the
captures behind it are not in the issue yet.

### Superior 6000S

TLV under `02 30 55`, decoded in
[`superior_status.cpp`](../components/levoit/superior_status.cpp): power, MCU
version, cover, tank, display, humidifying, target, humidity, temperature,
mode (Manual, Sleep, Humidity, Auto), fan speed 1..9, auto profile, dry mode
block, filter life, display lock. Commands under `02 xx 55` and `02 xx 50`,
built in [`superior_commands.cpp`](../components/levoit/superior_commands.cpp).
Already on the component; the only gap is that none of it is written up in
LEVOIT_UART.md.

## One entity set for all humidifiers

ESPHome has no humidifier domain, so the Superior maps the device onto the
`fan` entity: power, mist level as speed, operating mode as preset. The other
two follow that, so the Home Assistant panel
([levoit-esphome-ha](https://github.com/tuct/levoit-esphome-ha), with mvs90's
humidifier work in its PR #64, and gorischek's
[humidifier-card](https://github.com/gorischek/humidifier-card)) sees the same
entities on every model.

| Platform | `type` | Superior 6000S | LV600S | Dual 200S | State |
|----------|--------|----------------|--------|-----------|-------|
| `fan` | — | speeds 1..9; Manual, Sleep, Auto, Humidity, Dry | speeds 1..9 (app shows 3); Manual, Sleep, Humidity; Auto read-only | speeds 1..2; Manual, Auto | exists |
| `number` | `humidity_target` | yes | 40..80 | 30..80 | exists; bounds per model |
| `sensor` | `humidity`, `temperature` | yes | yes | yes | exists |
| `binary_sensor` | `water_tank_empty` | yes | shortage flag | suspected | exists |
| `binary_sensor` | `tank_removed` | `cover_open` today | yes | yes | **new**, or rename |
| `binary_sensor` | `humidifying` | yes | derived | mist level > 0 | exists |
| `switch` | `display` | yes | yes | yes, no status field | exists |
| `switch` | `child_lock` | yes | — | — | exists |
| `switch` | `auto_off` | — | — | yes, no status field | **new** |
| `select` or `number` | `warm_level` | — | 0..3 | — | **new** |
| `select` | `auto_profile`, `humidity_subtype`, `dry_level` | yes | — | — | exists |
| `number` | `timer` | hours | seconds LE32, same frames as Core | not seen | exists |
| `text_sensor` | `mcu_version` | yes | yes | yes | exists |
| `text_sensor` | `fault` | — | E1..E6 from offset 15 | — | **new** |

Two of the switches (display on the Dual, auto-off) have no status field in
the frames seen so far. The component keeps them at the last acknowledged
value, as gorischek's YAML does, and the README says so. A capture with each
toggled may yet find the byte.

## Steps

### 0. Shared-core step 3 first

[Step 3](./shared-core.md#3-split-levoitcpp) moves the decoders onto
`LevoitDevice *` and splits `levoit.cpp`. A humidifier decoder written before
that lands has to be rewritten in the merge, and the Core-family decoders are
exactly the files it touches. Everything that does not touch the component
(steps 1 and 2) can start now.

### 1. Collect what is missing (now)

* **Dual 200S, issue #66**: ask gorischek whether the YAML, teardown notes and
  photos may be integrated, credited to them; ask for the UART logs the decode
  came from (the YAML's receive-only mode prints both directions), one action
  per file: power on and off, each mist level, a target change, auto-off on and
  off, display on and off, tank off and on, a minute idle; the MCU version;
  and whether they will flash a test build.
* **LV600S, PR #152**: agree with mvs90 on the path from the separate
  component to `model: LV600S`, which the PR itself names as the goal. The
  owner's captures are not in the repo; the PR's native tests replay the
  payloads, which is the evidence the decode can cite until frames are
  committed under `devices/levoit-lv600s/uart/`.
* **Superior 6000S**: the Jyers protocol sheet from issue #10 and egoleos'
  frames from #78 go under `devices/levoit-superior-6000s/uart/`, so the
  decoder has a capture to cite like every other model.

### 2. Device folders

One shape, [`devices/levoit-superior-6000s/`](../devices/levoit-superior-6000s/)
as the template: `README.md` (quick facts, teardown, pins, both installs,
entities, credits), `common.yaml`, `<model>.yaml` and `_dev.yaml`, `-c3`
variants, builder files from `make-builder-yaml.ps1`, `uart/`,
`secrets-example.yaml`. `uart:` at 9600 for the LUH models. Photos with
permission, EXIF stripped.

* `devices/levoit-dual200s/` from the issue: six screws, two under the rubber
  feet, housing pulls off, coated board, XIAO on IO18 / IO19 with the stock
  module's `EN` to GND.
* `devices/levoit-lv600s/` comes with #152 (`HARDWARE.md`, `PROTOCOL.md`,
  wiring photos); it gets the common file layout when the model moves onto
  the component.

### 3. Component: the fixed-layout humidifier family

* `types.h`: `ModelType::LV600S`, `ModelType::DUAL200S`; `SwitchType::AUTO_OFF`;
  `BinarySensorType::TANK_REMOVED`; `SelectType::WARM_LEVEL` (or a number);
  `TextSensorType::FAULT`; `CommandType::requestHumidifierStatus`,
  `setAutoOffOn/Off`, `setWarmLevel`. `__init__.py`: both models in
  `VALID_MODELS`; the platforms get the new `type:` names.
* `humidifier_status.cpp`: `decode_lv600s_status()` (20 bytes, the table above)
  and `decode_dual200s_status()` (25 bytes), each with a `payload_len` check
  first, publishing through the existing `publish_*` and `apply_fan_status()`.
  Raw mode values differ per model and from the component's fan modes (component
  Auto is raw 2): one small per-model map, not a shared guess.
* `humidifier_commands.cpp`: the shared frames (power, display, Wi-Fi LED,
  timer) fall through to the Core builders; the family's own frames (mist level
  payload, warm level, humidity target per mode, auto-off, status request) are
  built here, guarded by model where the two differ.
* `decoder.cpp`: route `01 11 41` and the `0x12` reply to `01 10 41` for the
  LV600S, `01 C1 40` and the `0x12` reply to `01 C0 40` for the Dual 200S; add
  both to the status-frame predicate so dedup treats them as status.
* Polling: neither LUH model is known to push status unprompted. Reuse the Core
  300S status-request path (`requestCore300sStatus`, sent on a timer and after
  each command) with the model's request CMD.
* Command confirmation: #152 confirms setters from the next status, not the
  ACK alone, and drops pending writes on a tank, water or fault condition. Keep
  that as the behaviour for both LUH models; it is what the one hardware test
  verified.
* Nothing changes for the Superior 6000S or any purifier. Two ESPHome versions
  must build, the minimum in the component READMEs and the newest release.

### 4. Tests

* Frames from the captures into
  `components/levoit/tests/test_uart_integrity.py`: checksum, length, both
  layouts, a `0x12` reply that carries a status payload.
* The protocol and start-up tests from #152 ported into the host regression
  instead of a second test runner and workflow.
* Both LUH models in the compile matrix of `components/run-compile-tests.ps1`.

### 5. Docs and change log

* `LEVOIT_UART.md`: a **Humidifiers** chapter: the Superior TLV tags and
  commands (missing today), the fixed-layout family with both status layouts
  and the command table above, each row with the capture it rests on.
* `README.md`: the humidifier table gets a row per model with the install
  methods and the hardware status as reported, no more.
  `components/levoit/README.md`: the model list and the entity table.
* Fragments per PR, area "Levoit humidifiers" for the shared pieces, the model
  name for a single-model change.

### 6. Hardware tests

Owners run the `_dev` build and report entity by entity: jasongill and egoleos
(Superior 6000S), mvs90 (LV600S), gorischek (Dual 200S). Until a report
arrives a model says "compiles, not tested on hardware"; afterwards it says
what the report says.

## Decisions and open questions

* **`fan` stays the humidifier's main entity.** ESPHome has no humidifier
  component, the Superior already works this way, and the HA panel builds on it.
  The level and mode vocabulary differs per model; the YAML of each device
  only exposes what the model has.
* **Separate component or `model:`?** `model:`. The frame, counter, checksum,
  ACK handling, Wi-Fi LED, timer and most entities are shared; a second
  component (the #152 stepping stone) duplicates them and drifts. The
  stepping stone can merge first if that gets the LV600S to users sooner; the
  move onto the component then follows step 3.
* **Baud rate** stays a YAML setting; nothing in the component reads it.
* **Status without a push.** Whether the LUH MCUs push status on a panel button
  press, or only answer polls, decides how often the component asks. The
  captures from step 1 answer it.
* **Unknown bytes** in the Dual 200S status (5..8, 14..24): display, auto-off
  and water-shortage are the candidates, by analogy with the LV600S.
* **OasisMist LV450S / LV600S (US)**: likely the same family as the EU LV600S,
  but nobody has opened one; a request in the discussion thread.
