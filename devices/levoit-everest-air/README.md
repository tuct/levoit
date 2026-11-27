[← Back](../../README.md)

# Levoit Everest Air - Custom Firmware (ESPHome)

ESPHome on the Everest Air, through the `levoit` component: the features
below in Home Assistant, no cloud. Adding your own ESP32 is the recommended
route; flashing the original works too. The steps shared by all devices are in
the [install guide](../../docs/installing.md); this page has what is particular
to the Everest Air.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Everest Air |
| Tested MCU FW | 1.0.2 |
| ESP Module | ESP32-SOLO-1 |
| Board | xxx |
| Fan Speeds | 3 + Turbo |
| CADR (spec) | 612 m³/h |
| Noise | 24–56 dB |
| Room Size | 9–123 m² (97–1328 ft²) |
| ESPHome | 2026.6.0+ |
| PM Sensor | PM1.0 / PM2.5 / PM10 |

## Features

| Feature | Type | Notes |
|---------|------|-------|
| Fan | fan | 3 manual speeds + presets: Manual / Auto / Sleep / Turbo |
| Auto Mode | select | Default / Eco |
| Display | switch | Toggle LED display |
| Child Lock | switch | |
| Light Detect | switch | Auto-dims display when ambient light is low |
| PM1.0 | sensor | µg/m³ (3-channel PM monitor) |
| PM2.5 | sensor | µg/m³ |
| PM10 | sensor | µg/m³ (3-channel PM monitor) |
| AQI | sensor | As reported by MCU |
| Current CADR | sensor | m³/h, updated every 5s |
| Filter Life Left | sensor | % remaining |
| Filter Low | binary_sensor | On when < 5% |
| Filter Lifetime | number | Configurable in months |
| Reset Filter Stats | button | Resets CADR/runtime counters |
| Timer | number | Run timer in minutes |
| MCU Version | text_sensor | |
| Vent Angle | number | EverestAir-specific motorized louver, 45–90° |
| Cover Open | binary_sensor | Back/filter door open — unit powers off while open |

## Cover Open Sensor

The unit has a cover open sensor, to trick it if you want it running while open use a magnet opt mid like this:

![Board pinout](./images/cover_sensor_hack_1.jpg)

## 1. Build the firmware

**With Home Assistant (recommended):** [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha)
→ *Add device* → **Everest Air** → the original ESP, Seeed XIAO ESP32-C3 or
ESP32-S3. [Step by step](../../docs/installing.md#with-home-assistant-recommended).

**Manually:** the files in this folder, built with the ESPHome CLI or pasted
into the Device Builder. [Step by step](../../docs/installing.md#manually).

| Board | ESPHome CLI | Device Builder / web builder |
|-------|-------------|------------------------------|
| original ESP32-SOLO-1 | `levoit-everest-air.yaml` | `levoit-everest-air-builder.yaml` |
| Seeed XIAO ESP32-C3 | `levoit-everest-air-c3.yaml` | `levoit-everest-air-builder-c3.yaml` |
| Seeed XIAO ESP32-S3 | `levoit-everest-air-s3.yaml` | `levoit-everest-air-builder-s3.yaml` |

```bash
esphome run levoit-everest-air.yaml
```

## 2. Disassembly

Remove 3 black skrews from top

![Remove Skrews](./images/back_remove_skrews.jpg)

Carefully start to open from the back, pull up with help from some clips

![Open](./images/back_open.jpg)

Pull up left and right and remove the top

![Open](./images/opened.jpg)

### Debug Header Pinout

| Pin | Signal |
|-----|--------|
| 1 | EN (reset) |
| 2 | GND |
| 3 | 3.3V |
| 4 | TX |
| 5 | RX |
| 6 | IO0 |

## 3. Flash the original ESP32

Connect to the [debug header](#debug-header-pinout) with a USB-UART adapter (3.3V TTL), crossing TX/RX:
- Adapter TX →  RX
- Adapter RX →  TX

Connect **IO0 to GND before powering on** to enter bootloader mode.

Then [back up, flash and, if needed, restore](../../docs/installing.md#3-flash-the-original-esp32):

```bash
esptool read_flash 0 ALL levoit-everest-air-backup.bin
esphome run levoit-everest-air.yaml
```

> Note: backup may fail if watchdog-protected. Try while powered externally.

Check the [component README](../../components/levoit/README.md) for the UART
pin mapping per board.

## 4. Add your own ESP32 (recommended)

**Recommended modules:**
- [Seeed XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?sensecap_affiliate=3gGMlPv&referring_service=link)
- [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link)

Make sure to connect EN (reset) to GND on the Debug Header to dissable the old ESP32

Pinout: 

![Board pinout](./images/Uart_pins.jpg)
![Board pinout](./images/power_pins.jpg)

![Board pinout](./images/all_connected.jpg)

UART pads per config:

| Board | Config | UART pads |
|-------|--------|-----------|
| Seeed XIAO ESP32-C3 | `levoit-everest-air-c3.yaml` | D4 to TP82, D3 to TP33, as on a tested unit |
| Seeed XIAO ESP32-S3 | `levoit-everest-air-s3.yaml` | the same GPIO5 / GPIO6, on the S3 pads D4 (to TP82) and D5 (to TP33); untested |

[Wiring, parking the original, strapping pins](../../docs/installing.md#4-add-your-own-esp32)
in the install guide.

## 5. Assembly

Reverse the disassembly: put the top back on over its clips and refit the 3
black screws on top.

Reassemble and enjoy! [What to check](../../docs/installing.md#5-assembly)
once it is powered.

## Hardware & protocol notes

The serial protocol, the vent louver commands and the status TLV map are in
[PROTOCOL.md](./PROTOCOL.md).
