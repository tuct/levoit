[← Back to the project README](./README.md)

# Change Log

Project-level changes. Component releases have their own log:
[levoit](./components/levoit/README.md#change-log).

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
