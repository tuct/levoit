

# Project - ESPHome hacked Air Purifiers for Home Assistant

- Collection of air purifiers that can be more or less easily hacked to run ESPHome instead of cloud-based firmware.
- Eliminating cloud dependency and enabling native Home Assistant integration for air purifiers.


[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/P4P721ETB5)

I like Wi-Fi enabled air purifiers! But I hate the fact that they all depend on a cloud service.

I've run Home Assistant and custom-built ESPHome devices at home for quite some time, and found a project that let me flash custom, ESPHome-based firmware onto a Levoit Core 300S (which I already owned).
Wi-Fi enabled air purifiers often use an ESP32 for the Wi-Fi side plus a separate MCU to manage the purifier itself (speeds, air-quality sensor, auto mode, …), with a UART link between the two.


This started as a handful of custom ESPHome firmware and hardware projects for Levoit air purifiers, and is slowly growing into the broader collection you see here.

![header](./header.jpg)


## Overview of existing / tested esphome-ified Air Purifiers

Every purifier covered here at a glance — how it's converted to ESPHome, its specs, and how involved the teardown is. Click a model or its **Guide** for the full write-up.

| Model | Manufacturer | Methods | CADR (spec) | Noise | Disassembly | Guide | Links | Comments |
|-------|--------------|---------|-------------|-------|-------------|-------|-------|----------|
| [Core 200S](./devices/levoit-core200s) | Levoit | 🟢 Flash / 🔵 Add ESP | 167 m³/h | 24–48 dB | Easy | [Guide](./devices/levoit-core200s) | [Amazon](https://amzn.to/3SGH513) | ✅ Tested · 3 speeds, no air-quality sensor |
| [Core 300S](./devices/levoit-core300s) | Levoit | 🟢 Flash / 🔵 Add ESP | 214 m³/h | 24–50 dB | Easy | [Guide](./devices/levoit-core300s) | [Amazon](https://amzn.to/4aMVbnO) | ✅ Tested · PM2.5 + Auto mode |
| [Core 400S](./devices/levoit-core400s) | Levoit | 🟢 Flash / 🔵 Add ESP | 442 m³/h | 24–52 dB | Hard | [Guide](./devices/levoit-core400s) | [Amazon](https://amzn.to/4vOT9vt) | ✅ Tested · 4 speeds |
| [Core 600S](./devices/levoit-core600s) | Levoit | 🟢 Flash / 🔵 Add ESP | 641 m³/h | 26–54 dB | Hard | [Guide](./devices/levoit-core600s) | [Amazon](https://amzn.to/4opVx9z) | ✅ Tested · 4 auto modes |
| [Vital 100S](./devices/levoit-vital100s) | Levoit | 🟢 Flash / 🔵 Add ESP | 221 m³/h | 23–52 dB | Easy | [Guide](./devices/levoit-vital100s#teardown--disassembly) | [Amazon](https://amzn.to/3SaFron) | ✅ Tested · Pet mode; detailed teardown |
| [Vital 200S (Pro)](./devices/levoit-vital200s) | Levoit | 🟢 Flash / 🔵 Add ESP | 415 m³/h | 23–58 dB | Easy | [Guide](./devices/levoit-vital200s) | [Amazon](https://amzn.to/4xMiJn1) | ✅ Tested |
| [Everest Air](./devices/levoit-everest-air) | Levoit | 🟢 Flash / 🔵 Add ESP | 612 m³/h | 24–56 dB | Easy | [Guide](./devices/levoit-everest-air) | [Amazon](https://amzn.to/3Q1cMB) | ✅ Tested · vent louver, PM1.0/2.5/10, Turbo |
| [Sprout](./devices/levoit-sprout) | Levoit | 🟢 Flash / 🔵 Add ESP | 145 m³/h | 22–47 dB | Easy | [Guide](./devices/levoit-sprout) | [Amazon](https://amzn.to/4oAJs1n) | 🚧 WIP · white-noise audio (I2S MP3) |
| [LV PUR 131S](./devices/levoit-lv131s/) | Levoit | 🔴 Custom HW | — | — | Medium | [Guide](./devices/levoit-lv131s/) | — | Custom FW + MCU & sensor upgrade |
| [Levoit Mini](./devices/levoit-mini) | Levoit | 🔴 Custom HW | 78 m³/h | 41.8 – 53.6 dBA | Easy | [Guide](./devices/levoit-mini) | [Amazon](https://amzn.to/4acovEh) | Full custom PCB + 3D parts |
| [Philips / MUJI AC0650](./devices/philips-600-series) | Philips (Versuni) / MUJI | 🔵 Add ESP | 170 m³/h | 19–49 dB | Easy | [Guide](./devices/philips-600-series) | [Amazon](https://amzn.to/4vS5Ohs) | ✅ Tested · secure boot → replace module; no AQ sensor |
| [Philips / MUJI AC0651](./devices/philips-600-series) | Philips (Versuni) / MUJI | 🔵 Add ESP | 170 m³/h | 19–49 dB | Easy | [Guide](./devices/philips-600-series) | [Amazon](https://amzn.to/4elkSyg) | ✅ Tested · adds PM2.5 (PM1003) + Auto mode |
| [IKEA Förnuftig](https://edvoncken.net/2024/04/ikea-fornuftig-with-esphome/) | IKEA | 🔴 Custom HW | 120 m³/h | 28–60 dB | Easy | [Blog ↗](https://edvoncken.net/2024/04/ikea-fornuftig-with-esphome/) · [C6 ↗](https://github.com/horvathgergo/esp32c6-for-fornuftig) | — | 🔗 External · dumb 3-speed fan, ESP added for control |
| [IKEA Uppåtvind](https://github.com/jonathonlui/esphome-ikea-uppatvind) | IKEA | 🔴 Custom HW | 95 m³/h | 42.5–53.8 dB | Easy | [GitHub ↗](https://github.com/jonathonlui/esphome-ikea-uppatvind) | — | 🔗 External · small desk purifier, ESP added for control |

**Methods** — how the custom firmware ends up on the device:
- 🟢 **Flash** — flash ESPHome straight onto the device's own ESP32 (works where it isn't locked — most Levoits). Easiest; back up the stock firmware first.
- 🔵 **Add ESP** — wire in a separate ESP32 and disable the original (`EN`→GND). Needed when secure boot blocks reflashing, and fully reversible.
- 🔴 **Custom HW** — replace the controller board / build custom hardware.

**CADR** and **Noise** are manufacturer specs. **Disassembly** is a rough effort estimate (Easy / Medium / Hard) — check the linked **Guide** before you start.

## [ESPHome external component for Philips / MUJI Air Purifiers](./components/philips/README.md)

Supports the Philips-made (Versuni) **600 Series** sold under the MUJI brand — **AC0650/10** and **AC0651/10** — for now. It works much like the Levoit component, just speaking Philips' slightly different `FE FF` binary UART protocol. These units are secure-boot locked, so the approach is to add your own ESP32 and disable the original module.

**Requires:** ESPHome 2026.05.3+

### [Supported Models](./devices/philips-600-series)

| Model | MCU Version | Status | Notes | Amazon Link |
|-------|-------------|--------|-------|------|
| [Philips / MUJI AC0650](./devices/philips-600-series) | 0.1.9 | ✅ Tested | Fan, filters | [Amazon](https://amzn.to/4vS5Ohs) |
| [Philips / MUJI AC0651](./devices/philips-600-series) | ? | ✅ Tested | Adds PM2.5 (PM1003), allergen index, Auto mode, standby sensor | [Amazon](https://amzn.to/4elkSyg) |


## [Esphome external component for Levoit Air Purifiers](./components/levoit/README.md)

The Core and Vital Series share quite a lot on the protocol level, while having some differences based on model and MCU version.
This is an external ESPHome component that supports all (WIP!) Core and Vital Air Purifiers.

Can be flashed to the original ESP32-SOLO-C1 or also installed on top (replace original), [check 'Installation'](./components/levoit/README.md)

The Levoit Sprout additionally uses the [levoit_audio component](./components/levoit_audio/README.md) for white noise playback (MP3 from SPIFFS via I2S) — note its vendored `dr_mp3.h` build dependency.

**Requires:** ESPHome 2026.05.3+


### [Supported Models](./devices/README.md)

| Model | MCU Version | Status | Amazon Link |
|-------|-------------|--------|------|
| [Levoit Core 200s](./devices/levoit-core200s) | 2.0.11 | ✅ Tested |[Amazon](https://amzn.to/3SGH513)|
| [Levoit Core 300s](./devices/levoit-core300s) | 2.0.7, 2.0.11 | ✅ Tested  |[Amazon](https://amzn.to/4aMVbnO)|
| [Levoit Core 400s](./devices/levoit-core400s) | 3.0.0 | ✅ Tested  |[Amazon](https://amzn.to/4vOT9vt)|
| [Levoit Core 600s](./devices/levoit-core600s) | 2.0.1 | ✅ Tested |[Amazon](https://amzn.to/4opVx9z)|
| [Levoit Vital 100s](./devices/levoit-vital100s) | 1.0.5, 2.0.0(?) | ✅ Tested |[Amazon](https://amzn.to/3SaFron)|
| [Levoit Vital 200s (Pro)](./devices/levoit-vital200s) | 1.0.5, 2.0.0 Thanks @TheDave94 !|  ✅ Tested |[Amazon](https://amzn.to/4xMiJn1)|
| [Levoit Everest Air](./devices/levoit-everest-air) | 1.0.2 | ✅ Tested  |[Amazon](https://amzn.to/3Q1cMB)|

#### Missing /WIP

| Model | MCU Version | Status |
|-------|-------------|--------|
| [Levoit Sprout](./devices/levoit-sprout) | 1.0.5 |  🚧 WIP - Vital like + more| 
| [Levoit Core 400S-P Plasma Pro](./devices/xxx) | ??? | ??? |


### Other Models / Levoit Projects

* [Levoit LV PUR 131s](./devices/levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [Levoit Mini](./devices/levoit-mini) – Custom PCB, 3D parts, hardware hack

### Features

Core200s

![PCB back](./devices/levoit-core200s/images/controls_sensors.png)
![PCB back](./devices/levoit-core200s/images/config_diag.png)

Core300s - with Air Quality and Auto

![PCB back](./devices/levoit-core300s/images/filters.png)
![PCB back](./devices/levoit-core300s/images/config.png)

#### Fan

Native Home Assistant Fan component, with preset support.
Available speed levels and presets are based on model.

| Model | Speed Levels | Preset Modes |
|---------|------------|-------------|
| C200S | 1–3 | Manual, Sleep |
| C300S | 1–3 | Auto, Manual, Sleep |
| C400S | 1–4 | Auto, Manual, Sleep |
| C600S | 1–4 | Auto, Manual, Sleep |
| V100S | 1–4 | Auto, Manual, Sleep, Pet |
| V200S | 1–4 | Auto, Manual, Sleep, Pet |
| Sprout | 1–4 | Auto, Manual |
| EverestAir | 1–3 | Auto, Turbo, Manual |


#### Vent Angle & Cover (EverestAir)

The Everest Air adds a **motorized vent louver** and a **cover/door sensor** not present on the other models:

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| Vent Angle | number | `vent_angle` | Motorized louver angle, 45–90° (CMD `02 12 55`, status TLV `0x14`) **EverestAir only** |
| Cover Open | binary_sensor | `cover_open` | Back/filter door open — the unit powers itself off while open (TLV `0x15`) **Sprout + EverestAir** |

The vent angle is set as a number entity (45° = nearly closed/upward, 90° = fully open/forward). The MCU echoes the current angle back in status tag `0x14`, and it reads `0` while the unit is powered off.


#### Display / Light

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| Display | switch | `display` | Toggle the LED display on/off |
| Child Lock | switch | `child_lock` | Disable physical buttons on the device |
| Light Detect | switch | `light_detect` | Auto-dim display when ambient light is low **Vital Series + Core 600S + EverestAir** |
| Night Light | select | `nightlight` | Night light brightness: Off / Mid / Full **Only Core200S** |

#### Timer

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| Timer | number | `timer` | Run timer in minutes |
| Timer Set | text_sensor | `timer_duration_initial` | Originally set timer as readable string (e.g. "2h 30 min") |
| Timer Remaining | text_sensor | `timer_duration_remaining` | Time left on active timer (e.g. "1h 15 min") |

#### Filter Lifetime

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| Filter Lifetime | number | `filter_lifetime_months` | Expected filter lifespan in months (1–12); used to compute Filter Life % |
| Filter Life Left | sensor | `filter_life_left` | Remaining filter life as % ⁽¹⁾ |
| Filter Low | binary_sensor | `filter_low` | `on` when Filter Life % drops below 5% ⁽¹⁾ |
| Current CADR | sensor | `current_cadr` | Calculated Clean Air Delivery Rate at current fan speed in m³/h ⁽¹⁾ |
| Reset Filter Stats | button | `reset_filter_stats` | Reset cumulative CADR and runtime counters — restores Filter Life % to 100% ⁽¹⁾ |

> ⁽¹⁾ Computed by the component (not received from MCU), works on all models.

#### Auto Mode

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| Auto Mode | select | `auto_mode` | Auto mode type — options vary by model (see below) **Not for Core200S** |
| Auto Mode Room Size | number | `efficiency_room_size` | Target room area for efficient auto mode in m² **Not for Core200S / EverestAir** |
| Efficiency Counter | sensor | `efficiency_counter` | Seconds remaining at high fan speed in efficient auto mode **Vital only** |
| Auto Mode High Fan Time | text_sensor | `auto_mode_room_size_high_fan` | Time still running at high speed in efficient auto mode, human readable **Vital only** |

Auto mode options per model:

| Model | Options | Room Size Range |
|-------|---------|----------------|
| C200S | — | up to 40 m² (430 ft²) |
| C300S | Default / Quiet / Room Size | 9–50 m² (97–538 ft²) |
| C400S | Default / Quiet / Room Size | 9–38 m² (97–409 ft²) |
| C600S | Default / Quiet / Room Size / ECO | 9–147 m² (97–1,582 ft²) |
| V100S | Default / Quiet / Efficient | 9–52 m² (97–560 ft²) |
| V200S | Default / Quiet / Efficient | 9–87 m² (97–936 ft²) |
| Sprout| Default / Quiet / Efficient | 9–57 m² (97–936 ft²) |
| EverestAir| Default / Eco | — (no room-size setting) |

#### Air Quality Sensors

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| PM2.5 | sensor | `pm25` | Particulate matter concentration in µg/m³ from built-in sensor **Not for Core200S** |
| PM1.0 | sensor | `pm1_0` | Particulate matter concentration in µg/m³ from built-in sensor **only Sprout and EverestAir** |
| PM10 | sensor | `pm10` | Particulate matter concentration in µg/m³ from built-in sensor **only Sprout and EverestAir** |
| AQI | sensor | `aqi` | Air Quality Index as reported by the MCU **Not for Core200S** |

#### Info and Debug

| Feature | Type | Config Key | Description |
|---------|----|------------|-------------|
| MCU Version | text_sensor | `mcu_version` | Firmware version string of the purifier MCU chip |
| ESP Version | text_sensor | `esp_version` | ESPHome component version string |
| Error | text_sensor | `error_message` | Device error status: "Ok" or "Sensor Error" **Not for Core200S** |

### Change Log

#### ESP Version: 1.4.0 - 2026.06.14

* Added Levoit Everest Air support 
* Everest Air motorized vent louver as a number (45–90°, `vent_angle`, CMD `02 12 55`)
* Everest Air back/cover door sensor as a binary_sensor (`cover_open`, status tag `0x15`) — unit powers off while open



#### ESP Version: 1.3.1 - 2026.06.09

* ESPHome min version updated to **2026.5.3**
* Correct Core400S CADR and Room Size limits (@EdenNelson)
* Fix fan preset modes for newer ESPHome (`set_supported_preset_modes` moved to `FanTraits`) (@EdenNelson)
* Fix Core300S/400S falsely reporting "Sensor error" (removed incorrect status byte mapping)
* Add Vital 200S Pro support for MCU FW 2.0.0 with bulk-prefs SET (@TheDave94)
* LevoitSwitch: set has_state on publish to match Select/Number behavior (@TheDave94)
* Fix race condition where the led stays blinking even after conenction is restored (@Ahmed-max)



#### ESP Version: 1.3.0 - 2026.03.28

* Added Core 600S support: 4 fan speeds, 4 auto modes (Default / Quiet / Room Size / ECO), Light Detect switch, CADR 641 m³/h
* Added Vital 200S (Pro) support: same protocol as Vital 100S, tested with original ESP
* Updated Auto Mode select to show model-specific options (3 for Core/Vital, 4 for Core 600S)


#### ESP Version: 1.2.0-esphome - 2026.03.22

* Added Core200s support and readme / example.
* Renamed/moved repo to tuct/levoit - for easier collaboration with pull requests, ...

#### 2026.03.21

* Works with ESPHome 2026.3+
* Sensors - added state class measurement -> allow statistics to be tracked

#### 2026.01.15 ESPHome 2025.12.5+ Compatibility

* The component has been updated for ESPHome 2025.12.5


## Contributing

### Adding a New Device or Firmware Version

If you have a Levoit model that isn't supported yet, or a newer MCU firmware on a supported model, a UART dump is the best way to contribute.

What's needed:
- A logic analyzer capture of the UART traffic between the ESP32 and the MCU (both directions)
- The MCU firmware version (visible in the Levoit app or via `mcu_version` sensor once flashed)
- A description of any features the device has (fan speeds, auto modes, sensors, lights, etc.)

Open an issue or pull request in the repo with the dump attached.

### Capturing a UART Dump

The ESP32 and MCU communicate over UART at **115200 baud, 8N1**. See [Levoit UART Protocol Details](./LEVOIT_UART.md) for a full description of the packet format. To capture traffic:

> Check the individual device README for teardown steps, PCB photos, and the exact solder points to use for your model.

1. Open the device and locate the correct solder points — see the individual device README for the exact pads
   > **Note:** The debug pin header RX/TX pins are used to flash the ESP32. They are **not** the UART line between the ESP32 and the MCU. You need to tap into the dedicated ESP↔MCU communication pads (test points or vias near the ESP32), not the header.
2. Connect a logic analyzer to both the **ESP TX** and **MCU TX** lines, with a shared GND
3. In **Saleae Logic 2**, add an **Async Serial** analyzer on each channel:
   - Baud rate: `115200`
   - Bits per frame: `8`
   - Stop bits: `1`
   - No parity
4. Power on the device and capture separate, clearly labelled dumps for each action — one action per capture makes it much easier to identify which bytes correspond to which command:

   | Dump | Action |
   |------|--------|
   | `bootup` | Power on → wait until Wi-Fi connected and app shows online |
   | `speed_1-4_app` | Switch through fan speeds 1 → 2 → 3 → 4 via the **app** |
   | `speed_1-4_device` | Switch through fan speeds 1 → 2 → 3 → 4 via the **physical buttons** |
   | `mode_app` | Switch through all modes (Manual / Auto / Sleep / Pet) via the **app** |
   | `mode_device` | Switch through all modes via the **physical buttons** |
   | `auto_mode` | Switch through all auto mode sub-options (Default / Quiet / Room Size / etc.) |
   | `display_on_off` | Toggle the display on and off |
   | `child_lock` | Enable and disable child lock |
   | `timer` | Set a timer via the app |
   | `filter_reset` | Reset filter stats |
   | *(model-specific)* | Any unique features: lights, white noise, CO₂ sensor, etc. |

   Label each file clearly (e.g. `core300s_2.0.11_speed_app.txt`).

   > **Example:** See [`devices/levoit-sprout/uart/uart_dumps`](./devices/levoit-sprout/uart/uart_dumps) for a real Sprout dump covering boot, speed switching, light modes, and white noise — each section labelled with the action performed.

5. In Logic 2, use **Export Data** → export the analyzer results as a text/CSV file (not the `.sal` session). A plain text file with the decoded HLA output is all that's needed.

### Decoding with the Logic 2 HLA

A High-Level Analyzer for the Levoit UART protocol is included in [`logic2/levoit_uart/`](./logic2/levoit_uart/).

**Install:**
1. Open Logic 2 → **Extensions** (puzzle icon) → **Load Existing Extension**
2. Select the `logic2/levoit_uart/` folder

**Use:**
1. Add an **Async Serial** analyzer on the **ESP TX** channel (115200 baud, 8N1) — this is ESP→MCU traffic
2. Add a second **Async Serial** analyzer on the **MCU TX** channel — this is MCU→ESP traffic
3. Add a **Levoit UART Extractor** HLA on top of the first Async Serial, set **Channel** to `ESP->MCU`
4. Add a second **Levoit UART Extractor** HLA on top of the second Async Serial, set **Channel** to `MCU->ESP`
5. Decoded packets appear as: `[MCU->ESP] RESP(0x52) |  CMD=01 40 41  |  PAY=00 01 ...`

Both HLAs run side by side so you can see the full request/response exchange in one view.

See [`logic2/levoit_uart/README.md`](./logic2/levoit_uart/README.md) for full details.

## Info
Not my projects, but worth checking out:
* [levoit-vital-200s + levoit-vital-200s pro, levoit-vital-100s?](https://github.com/targor/levoit_vital/?tab=readme-ov-file)
* https://github.com/mulcmu/esphome-levoit-core300s
* https://github.com/acvigue/esphome-levoit-air-purifier

## Helpful links
* [How to open Levoit's](https://www.youtube.com/watch?v=6wxHpUVcGFc)
* [How to open Levoit's smaller](https://www.youtube.com/watch?v=rAjLNR1jQkw)

## Details about generic protocol / etc
[Levoit UART Protocol Details](./LEVOIT_UART.md)
