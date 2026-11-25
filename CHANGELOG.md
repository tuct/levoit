[← Back to the project README](./README.md)

## Unreleased — UART integrity

- Decode both length bytes, discard impossible/truncated frames and preserve the incoming sequence in ACKs. Reject foreign-version/error status envelopes on Core 600S and Vital 100S. Host regression tests use the actual receive and ACK methods. Hardware verification of this current upstream revision is pending.

# Change Log

Project-level changes. Component releases have their own log:
[levoit](./components/levoit/README.md#change-log).

### Unreleased

* **Core 300S with the `01 32 40` status** (a LAP-C302S-WGC, MCU 1.0.0) now
  reports its state: the 19-byte status is decoded and asked for at boot, a
  filter reset on the panel (Sleep held 3 s) resets the filter life, and a
  new `power_button_held` binary sensor (Core models) follows a long press of
  the On/Off button. Capture in
  [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)
  ([#140](https://github.com/tuct/levoit/pull/140),
  [#141](https://github.com/tuct/levoit/pull/141),
  [#142](https://github.com/tuct/levoit/pull/142),
  [#143](https://github.com/tuct/levoit/pull/143))
* **Core plasma documented** in [LEVOIT_UART.md](./LEVOIT_UART.md#plasma-01-e8-a5):
  the plasma command `01 E8 A5`, read from the stock firmware of a Core 300S
  (not sent to a purifier yet), and offset 6 of the `01 32 40` status, which
  follows the panel's plasma button. Not used by the component yet
  ([#144](https://github.com/tuct/levoit/pull/144))
* **Room size: one number to set** (Core 300S / 400S / 600S, Vital 100S /
  200S). Changing *Auto Mode Room Size Preset* while Room Size is the active
  auto mode now sends it to the purifier at once; before, it only took effect
  the next time Room Size was picked. *Auto Mode Room Size*, which shows what
  the purifier reports (0 outside Room Size), moves to *Diagnostic* in Home
  Assistant. Same entity, same name: automations keep working. The Sprout
  keeps it as a setting, since it has no Preset
  ([#137](https://github.com/tuct/levoit/pull/137))
* **Statistics for the remaining measurements**: the Superior 6000S humidity,
  temperature and MCU filter life, and the Philips filter sensors, get
  `state_class: measurement`, so Home Assistant keeps long-term statistics for
  them, as it already did for PM, AQI, CADR and filter life. The Superior ones
  also get their unit and device class (%, °C) ([#138](https://github.com/tuct/levoit/pull/138))

### 2026.11.23 (component 1.7.3)

* **Component [1.7.3](./components/levoit/README.md#change-log)**: the
  `philips` UART check moves to config validation, ahead of ESPHome 2027.3.0
  ([#129](https://github.com/tuct/levoit/pull/129)); `levoit` code unchanged
* **Eleva 300X runs on real hardware**: first install with a replacement Seeed
  XIAO ESP32-C3 (RX GPIO5 = D3 / TX GPIO4 = D2 on the stock module's MCU link), fan speed
  and PM / AQI confirmed. The stock-module config is still untested. The guide
  now has the disassembly steps, the board pins labelled, the wiring at both
  ends and where the XIAO goes ([#135](https://github.com/tuct/levoit/pull/135))
* **Superior 6000S on its stock ESP32-C3-SOLO-1**: the configs used `esp32dev`
  with GPIO16 / GPIO17, a guess carried over from the purifiers. They now use
  the C3 module with the MCU link on GPIO18 / GPIO19, as egoleos confirmed on an
  LEH-S601S-WUS ([#78](https://github.com/tuct/levoit/discussions/78)); DIO
  flash, the log on UART0, ESPHome 2026.6.0 or newer. The README has the
  flashing steps (GPIO9 low for download mode)
  ([#128](https://github.com/tuct/levoit/pull/128))
* **Seeed XIAO ESP32-S3 configs for the remaining models**: Everest Air,
  Superior 6000S, Eleva 300X, Philips 600 series (AC0650) and 900 series
  (AC0950), with builder files and Add device templates. Everest Air keeps the
  GPIO numbers of its XIAO C3 config; the Philips configs keep the XIAO C3's
  pads (GPIO20 / GPIO21 / GPIO10 are not on the XIAO S3). All untested on
  hardware so far. Philips builder files are named `…-builder-s3.yaml` like the
  Levoit ones ([#127](https://github.com/tuct/levoit/pull/127))
* **Seeed XIAO ESP32-C3 replacement configs for the Eleva 300X and Superior
  6000S**, the two models that had the original (stock C3 module) and a XIAO
  ESP32-S3 replacement but no XIAO C3 one. Wired to the stock module's MCU link
  (pins 27 / 28), RX GPIO5 / TX GPIO4, with builder files and Add device
  templates. The Eleva one works on hardware (above); the Superior one is
  untested. Every Levoit model now offers original,
  XIAO ESP32-C3 and XIAO ESP32-S3 ([#134](https://github.com/tuct/levoit/pull/134))
* **Docs**: Core 400S CORE400S Ctrl V1.2 pads, 5 V from J4 and opening it
  without extensions ([#130](https://github.com/tuct/levoit/pull/130)); flashing
  the Core 400S over the air without opening it, a third-party proof of concept,
  cross-linked from the Core 300S ([#132](https://github.com/tuct/levoit/pull/132));
  Core 200S-P flash header photos ([#133](https://github.com/tuct/levoit/pull/133));
  Philips AC0650 wiring photos and ESP32-S3 pins
  ([#131](https://github.com/tuct/levoit/pull/131), by fgibertoni); a current
  screenshot of the Levoit ESPHome sidebar page ([#125](https://github.com/tuct/levoit/pull/125))

### 2026.11.22 (component 1.7.2)

* **Levoit component [1.7.2](./components/levoit/README.md#change-log)**: Vital
  timer, filter wear, Wi-Fi LED and daytime fan mode fixes, `dark_detected`
  shown the right way round
* **Device configs report their model**: every device config and builder file
  has the `model` text sensor (component 1.7.1); the Philips configs also get
  the `esp_version` sensor. The Levoit ESPHome integration reads both
  ([#108](https://github.com/tuct/levoit/pull/108))
* **Vital 100S / 200S configs in line with each other**: room-size limits
  9–52 m² / 9–87 m², `efficiency_counter` and `dark_detected` added, the Vital
  100S logger at `DEBUG` instead of `VERBOSE`, the Vital 200S stock configs get
  `flash_mode: dio` and `min_version: 2026.6.0` like the 100S
  ([#117](https://github.com/tuct/levoit/pull/117), by wasdennnoch)
* **Vital 100S and Eleva 300X: flash at 80 MHz** (DIO, as before), as the stock
  firmware runs it. This replaces the 40 MHz from 1.7.1: tested at 80 MHz on a
  Vital 100S and a Vital 200S, and the Eleva's stock image headers set DIO at
  80 MHz ([#117](https://github.com/tuct/levoit/pull/117), [#119](https://github.com/tuct/levoit/pull/119))
* **Device builder templates** (`devices/templates.json`) for the Levoit
  ESPHome integration's *Add device*: original ESP, XIAO ESP32-C3, ESP32-C3 and,
  for the Core and Vital models, ESP32-S3 ([#109](https://github.com/tuct/levoit/pull/109),
  [#121](https://github.com/tuct/levoit/pull/121)); S3 needs the integration 0.4.1 or newer
* **S3 configs for the Seeed XIAO ESP32-S3**: the Core 200S / 300S / 400S / 600S
  and Vital 100S / 200S S3 configs use `board: seeed_xiao_esp32s3` instead of the
  generic DevKitC-1. Same GPIO numbers, so existing wiring keeps working; the
  pin lines name the XIAO pads (GPIO4 = D3, GPIO5 = D4, GPIO6 = D5)
  ([#123](https://github.com/tuct/levoit/pull/123))
* **CI**: the builder-file check builds only the devices a PR touches (all of
  them for component changes), up to five files per job
  ([#122](https://github.com/tuct/levoit/pull/122))

### 2026.11.22

* **Levoit component [1.7.1](./components/levoit/README.md#change-log)**: the
  version carries the build commit, a new `model` text sensor, the Everest Air
  set-speed fix, no ESP32-C3 compiler warnings; ESPHome 2026.6.0 or newer
* **`model` text sensor** for `levoit` and `philips`, reporting the configured
  model ([#104](https://github.com/tuct/levoit/pull/104)); the device configs
  get it in the next PR, now that it is released
* **Minimum ESPHome is now 2026.6.0** (CI, READMEs, the esphome-devices configs)
  ([#105](https://github.com/tuct/levoit/pull/105))
* **Vital 100S / Eleva 300X on the stock ESP32-C3-SOLO-1: DIO at 40 MHz again
  with current ESPHome.** The override was set through `platformio_options`,
  which ESPHome's native ESP-IDF build ignores, leaving the board default
  (QIO, 80 MHz) that can fail to boot on that module. They now use
  `esp32: flash_mode: dio` / `flash_frequency: 40MHz` (ESPHome 2026.6.0+) and
  set `min_version: 2026.6.0`; the Vital 100S `_dev` config gets them too
* **C3 / S3 replacement configs** (and the Mini): removed a copied
  `platformio_options` block (`BOARD_HAS_PSRAM`, `qio_opi`, QIO / 80 MHz). It
  only applies to Arduino builds and the board defaults, and produced
  deprecation / ignored warnings
* **One version for the repo.** `philips` now reports the same version as
  `levoit` (from `components/levoit/VERSION`) and has an `esp_version` text
  sensor, e.g. `1.7.1+3fa21c4 esphome` ([#100](https://github.com/tuct/levoit/pull/100))
* **`stable` branch**: a release workflow moves it to every published release,
  so a config can pull the component with `ref: stable` and get releases only
  ([#100](https://github.com/tuct/levoit/pull/100))
* **Builder files follow `stable`** and are checked by CI: the `Builder files`
  workflow fails when one does not match its device yaml / `common.yaml`, and
  compiles every one. New builder files for the Eleva 300X, Everest Air,
  Superior 6000S and Philips AC0650 / AC0950; `make-builder-yaml.ps1` no
  longer garbles `—`, handles the Philips configs and has a `-Check` mode
  ([#101](https://github.com/tuct/levoit/pull/101)). They fetch the component on
  every build (`refresh: always`), so a build right after a release gets it
  ([#102](https://github.com/tuct/levoit/pull/102))
* **Everest Air on a Seeed XIAO ESP32-C3**: `levoit-everest-air-c3.yaml` and its
  builder file, pins D4 / D3 as on a tested unit
  ([#103](https://github.com/tuct/levoit/pull/103))
* **Core 600S:** the README pointed original-ESP users at
  `levoit-core600s-builder-solo-1c.yaml`, which has no `esphome:` section and
  does not validate. It is removed; use `levoit-core600s-builder.yaml`
  ([#101](https://github.com/tuct/levoit/pull/101))

### 2026.11.21

* **Levoit component [1.7.0](./components/levoit/README.md#change-log)** — the
  Eleva 300X, ESPHome 2026.11 support and Ethernet boards
* **New device: Levoit Eleva 300X** (`model: ELEVA300X`) — Vital protocol on
  the stock ESP32-C3, decoded from 19 Saleae captures of the stock firmware:
  fan levels 1–4, auto/sleep, PM1.0/2.5/10 and AQI, mute, AQI ring brightness,
  the MCU-side quiet period (light or time window, with local-time sync),
  adaptive start, dynamic manual/sleep, timer and filter reset. MCU UART on
  GPIO18 / GPIO19, the same ESP32-C3-SOLO-1 module and pins as the Vital
  100S / 200S. Compiles, **not tested on hardware yet**. See
  [devices/levoit-eleva-300x](./devices/levoit-eleva-300x)
  ([#95](https://github.com/tuct/levoit/pull/95))
* **ESPHome 2026.11 compatibility** for both components, still building on the
  minimum 2026.5.3 (@Scags104, [#93](https://github.com/tuct/levoit/pull/93))
  * `format_hex_pretty()` returning a `std::string` was removed in 2026.11; the
    levoit and philips UART logs now use `format_hex_pretty_to()` (available
    since 2026.3)
  * Sprout light: `add_effects()` takes a pointer and a count since 2026.11 and
    keeps the pointer, so the breathing effect table is now a member; older
    releases still get the initializer-list call
* **CI builds every config against the newest ESPHome and the minimum
  (2026.5.3)**, so a fix for a new release cannot silently break older ones
* **Run the CI compile tests locally** with `components/run-compile-tests.sh`
  (macOS / Linux) or `components\run-compile-tests.ps1` (Windows); `min` builds
  against the minimum version in its own venv. See
  [CONTRIBUTING](./CONTRIBUTING.md#ci)
* **Levoit component: Ethernet boards build.** A config without the `wifi`
  component no longer fails on `wifi/wifi_component.h`; the Wi-Fi LED now shows
  the network state on Wi-Fi and Ethernet alike, with a new `test-ethernet`
  compile test (@benedikt-bartscher, [#86](https://github.com/tuct/levoit/pull/86))
* **Vital 100S:** the MCU UART for an added ESP is on test pads TP57 / TP64,
  not on the debug header (@benedikt-bartscher,
  [#87](https://github.com/tuct/levoit/pull/87))
* **Core 200S-P:** the MCU UART pins are swapped compared with the Core 200S,
  and the board should be powered from +5V while flashing (@njanik,
  [#88](https://github.com/tuct/levoit/pull/88))
* **Vital 200S:** a standalone install guide for an added ESP, tested with a
  Seeed XIAO ESP32-C6 on a Vital 200S-P (@bcrook,
  [#90](https://github.com/tuct/levoit/pull/90))
* **Core 200S / 400S:** the two install routes and their serial pads are now
  documented separately (@gemivnet, [#94](https://github.com/tuct/levoit/pull/94))

### 2026.10.25

* **Levoit component [1.6.0](./components/levoit/README.md#change-log)** — the
  Core 200S filter life finally works, and the 1.5.0 attempt at it is reverted.
  Three separate bugs, found with the help of the reports and UART captures in
  [#60](https://github.com/tuct/levoit/issues/60)
  * The per-minute CADR accumulation **truncated to zero below 60 m³/h**, so a
    Core 200S on low or Sleep — and a Sprout on speed 1 — accumulated nothing
    at all and sat at 100% for months
  * A further 25% undercount from a 4-speed divisor on a 3-speed unit, and the
    Superior 6000S lost speeds 5–9 from the calculation entirely
  * ⚠️ **1.5.0 is superseded for the Core 200S**: it read the wrong status byte
    as a percentage, reporting 0.0% on some firmwares and a constant 100% on
    others
  * Status bytes 6 and 7 **swap roles between MCU firmware versions**, so the
    decoder now matches on value rather than position — the full byte map and
    both layouts are in
    [devices/levoit-core200s](./devices/levoit-core200s/README.md)
* **Core 300S: a no-disassembly flashing route.** Added
  [nico202/levoit-core300s-ota-wifi-flash](https://github.com/nico202/levoit-core300s-ota-wifi-flash)
  to the device page — a third-party proof of concept that gets ESPHome onto
  the stock ESP32 by hijacking the VeSync OTA on the LAN, with no teardown and
  no soldering. The teardown section now opens with a table of all three routes
* **Core 400S: board `Core400S_C_V2.2P2.2SP3` confirmed working** on the
  existing config (MCU 2.0.18), so the page lists confirmed revisions rather
  than implying V1.2 is the only one. Plus a warning to keep the backup baud
  low, and a note that the board can be flashed outside the unit
  ([#79](https://github.com/tuct/levoit/issues/79))
* **Vital 100S: an "Install New ESP32" section**, which this page was missing
  entirely — built around an install photo contributed in
  [#72](https://github.com/tuct/levoit/issues/72), showing an external FPC
  antenna and the wiring to the debug header. The auto-mode fan speed reading
  25% at the lowest rate is recorded there as a known issue, with the
  datapoint it most likely comes from

### 2026.09.26

* **Levoit component 1.5.0** — Superior 6000S support, the `filter_low` device
  class fix, and build fixes for optional platforms.
  ⚠️ **Superseded for the Core 200S by 1.6.0**, see above
* **Compile tests in CI.** Both components gained a `tests/` folder of
  self-contained configs, run by GitHub Actions on every pull request that
  touches `components/`. They caught two real build failures on their first run
* **The root README split into focused docs** — it had grown to ~650 lines.
  Device and component specifics moved to their own pages, leaving an overview,
  plus new [CONTRIBUTING.md](./CONTRIBUTING.md), this change log,
  [adding a new device](./docs/adding-a-new-device.md) and
  [capturing a UART dump](./docs/capturing-uart.md)
* **FCC internal photos** documented as a way to check a PCB *before* buying or
  opening a device — most Levoit filings are under grantee
  [`2ARBY`](https://fccid.io/2ARBY) and include photos of the bare board

### 2026.09.12

* Levoit component **1.4.3** — `filter_low` corrected to a `problem` binary
  sensor rather than a `battery` one, so a spent filter shows as a problem in
  Home Assistant
* Added the Silo City Labs ESP32-C6 replacement controller for the non-smart
  **Core 300-P**, which has no Wi-Fi module to convert
* Vital 100S: use `board_build.f_flash` for the 40 MHz override

### 2026.09.11

* **Philips Series 900 — an AC0951 now runs ESPHome.** The MCU↔module protocol
  is decoded and turns out to be the *same* one the
  [`philips`](./components/philips/README.md) component already speaks for the
  AC0650/AC0651 — 115200 8N1, `FE FF` framing, CRC-16/CCITT-FALSE, almost the
  same datapoint map. `model: AC0950` / `AC0951` added; every write frame was
  verified byte-for-byte against logic-analyzer captures before it was flashed
  ([devices/philips-900-series](./devices/philips-900-series))
  * New entities the 900 adds: **child lock**, **beep**, **display brightness**
    (off / low / bright) and a **sleep timer** in hours with a minutes-remaining
    sensor
  * Two values differ from the 600 series: medium fan mode is `0x13` (not
    `0x01`) and the HEPA filter total is 9600 (not 4800)
  * Wiring guide with annotated pads, the 10k pull-up measured on the module's
    reset pin, the XIAO ESP32-C3 pin mapping (`D7` RX / `D10` TX, avoiding the
    C3's strapping pins) and an install walkthrough with photos
  * ⚠️ The **AC0950** is still unverified — every capture and the working
    install are from an AC0951
* **Levoit Superior 6000S humidifier support** — the first non-purifier in the
  repo. It speaks the same MCU protocol as the Levoit air purifiers, so it is a
  new `model: SUPERIOR6000S` on the existing
  [`levoit`](./components/levoit/README.md) component rather than a new one.
  Ported from [Jyers/esphome-projects](https://github.com/Jyers/esphome-projects)
  (thanks [@Jyers](https://github.com/Jyers) for the reverse-engineering)
  ([devices/levoit-superior-6000s](./devices/levoit-superior-6000s))
  * ⚠️ Compiles and its frames match the captures it came from, but **not run on
    hardware here** — nobody in this repo owns a Superior 6000S
* Added a **Humidifiers** section to the overview above, alongside the purifiers
* Fixed a latent build bug in both the `philips` and `levoit` components:
  platform includes were guarded on the generic `USE_SWITCH` / `USE_SENSOR` /
  … macros, which *any* component defines, so a config with an unrelated
  switch/select platform failed to compile

### 2026.09.09

* Added a [Philips Series 900](./devices/philips-900-series) research folder — AC0950 / AC0951: teardown notes, board observations, a reverse-engineering log and a passive both-direction UART sniffer config
* **Module identified: the Series 900 uses an MXCHIP `EMC6069-P`** (Wi-Fi + BLE, FCC ID [`P53-EMC6069`](https://fccid.io/P53-EMC6069)) — *not* an EMW3080, so the LibreTiny reflash route does not carry over. The working plan there is 🔵 *Add ESP*; the MCU protocol is still undecoded

### 2026.09.08

* Levoit component **1.4.1** — compiler-warning cleanup for the ESP-IDF build, plus a `total_runtime` log-label fix (@EdenNelson, #59; details in the [component change log](#change-log---levoit-component))

### 2026.09.07

* Levoit component **1.4.1** — `fan_operating_mode` select, coherent fan mode commands, Core room size round-trip fix, fan-speed fix when leaving a preset, and `auto_profile_room_size_input` (details in the [component change log](#change-log---levoit-component))
* Added "Cloud-free without ESPHome" section — Philips / Versuni MXCHIP models controllable locally over CoAP
* Research note: the MXCHIP EMW3080 is a relabelled Realtek RTL8710BN, a LibreTiny/ESPHome target — so those Philips models may be reflashable rather than needing an added ESP32 (unverified, needs a teardown)
* Overview table reworked: new **Support** column separating in-repo components from external projects, rows grouped by manufacturer
* Removed the `clock_clock` and `lvgl_clock` components — split out into their own repo

### 2026.08.29

* Added MIT License

### 2026.08.13

* Fixed fan speed not being sent when leaving a preset at an unchanged level (@Bleialf, #53)

### 2026.07.20

* LV-PUR 131: added PMS5003 and DHT22 support (@X3NOOO, #52)
* Added `fan_operating_mode` select for dashboards that don't render fan presets (@EdenNelson, #50)

### 2026.07.04

* Added Levoit LV-PUR 131 support (@X3NOOO, #51)
* Fixed Core room size round trip (@EdenNelson, #46)
* Made Manual/Auto fan mode commands coherent (@EdenNelson, #48)

### 2026.06.20 

* Added Philips Series 600 Support
* Rework started to esphome hacked air purifiers from free levoit project
* added Links to Ikea hacks

### 2026.06.14

* Added Levoit Everest Air via Levoit component 
