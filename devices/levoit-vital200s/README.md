[← Back](../../README.md)

# Levoit Vital 200S (Pro) - Custom Firmware (ESPHome)

Same protocol as the Vital 100S. Tested with original ESP by @dnsefe.

ESPHome on the Vital 200S and 200S Pro, through the `levoit` component: the
features below in Home Assistant, no cloud. Adding your own ESP32 is the
recommended route; flashing the original ESP32-C3 works too. The steps shared
by all devices are in the [install guide](../../docs/installing.md); this page
has what is particular to the Vital 200S.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Vital 200S / Vital 200S Pro |
| Tested MCU FW | 1.0.5 |
| ESP Module | ESP32-C3-SOLO-1 |
| Board | TODO |
| Fan Speeds | 4 |
| CADR (spec) | 415 m³/h |
| Noise | 23–58 dB |
| Room Size | 9–87 m² (97–936 ft²) |
| ESPHome | 2026.6.0+ |
| PM Sensor | PM1003 |

## Features

| Feature | Type | Notes |
|---------|------|-------|
| Fan | fan | 4 speeds, presets: Manual / Auto / Sleep / Pet |
| Auto Mode | select | Default / Quiet / Efficient |
| Auto Mode Room Size | number | 9–87 m² |
| Auto Mode High Fan Time | text_sensor | Remaining high-speed runtime in efficient mode |
| Efficiency Counter | sensor | Seconds remaining at high fan speed |
| Display | switch | Toggle LED display |
| Child Lock | switch | |
| Light Detect | switch | Auto-dims display when ambient light is low |
| PM2.5 | sensor | µg/m³ |
| AQI | sensor | As reported by MCU |
| Current CADR | sensor | m³/h, updated every 5s |
| Filter Life Left | sensor | % remaining |
| Filter Low | binary_sensor | On when < 5% |
| Filter Lifetime | number | Configurable in months |
| Reset Filter Stats | button | Resets CADR/runtime counters |
| Timer | number | Run timer in minutes |
| MCU Version | text_sensor | |
| Error | text_sensor | "Ok" or "Sensor Error" |

## Home Assistant Device Dashboard

### Fan
<img width="601" height="620" alt="image" src="https://github.com/user-attachments/assets/f5a88bef-3bc6-46f4-8645-e4db452c200d" />

### Sensors
<img width="411" height="522" alt="image" src="https://github.com/user-attachments/assets/3e8132f2-d138-42f1-a4b3-b448a05de7c1" />

### Configuration
<img width="290" height="763" alt="image" src="https://github.com/user-attachments/assets/14f3005e-3241-49c4-827e-9552cf4f7211" />

## 1. Build the firmware

**With Home Assistant (recommended):** [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha)
→ *Add device* → **Vital 200S** → the original ESP, Seeed XIAO ESP32-C3 or
ESP32-S3. [Step by step](../../docs/installing.md#with-home-assistant-recommended).

**Manually:** the files in this folder, built with the ESPHome CLI or pasted
into the Device Builder. [Step by step](../../docs/installing.md#manually).

| Board | ESPHome CLI | Device Builder / web builder |
|-------|-------------|------------------------------|
| original ESP32-C3-SOLO-1 | `levoit-vital200s.yaml` | `levoit-vital200s-builder.yaml` |
| Seeed XIAO ESP32-C3 | `levoit-vital200s-c3.yaml` | `levoit-vital200s-builder-c3.yaml` |
| Seeed XIAO ESP32-S3 | `levoit-vital200s-s3.yaml` | `levoit-vital200s-builder-s3.yaml` |

```bash
esphome run levoit-vital200s.yaml
```

## 2. Disassembly

* Remove the front filter cover
* Remove 4 phillips screws
  * <img width="2304" height="1118" alt="image" src="https://github.com/user-attachments/assets/ed008acd-d4b0-4e55-b0e3-2e4bc2d6aabe" />

* Pry the white trim/slide the top black towards you
  > I broke the clips by prying the white trim pieces out then sliding the black control panel grille forward; but it worked
* The wires are very short so try to unplug + unclip.
  * <img width="500"  alt="image" src="https://github.com/user-attachments/assets/f3fbd137-6c23-4c41-bb4f-2928d6935798" />

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
- Adapter TX → MCU RX
- Adapter RX → MCU TX

Connect **IO0 to GND before powering on** to enter bootloader mode.

Then [back up, flash and, if needed, restore](../../docs/installing.md#3-flash-the-original-esp32):

```bash
esptool read_flash 0 ALL levoit-vital200s-backup.bin
esphome run levoit-vital200s.yaml
```

> Note: backup may fail if watchdog-protected. Try while powered externally.

Check the [component README](../../components/levoit/README.md) for the UART
pin mapping per board.

## 4. Add your own ESP32 (recommended)
> Tested by @bcrook on a Seeed Xiao ESP32-C6 + Vital 200S-P

**Recommended modules:**
- [Seeed XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?sensecap_affiliate=3gGMlPv&referring_service=link)
- [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link)

<img width="2304" height="958" alt="image" src="https://github.com/user-attachments/assets/b7235e33-998f-4eea-ae13-3d3470a87ece" />

* Bridge EN and GND on debug header to force onboard ESP32 into reset/off state
* Run power for new ESP32
  * 5V from lower debug header to V-BUS on ESP32
  * or, 3.3V from right (ESP32) debug header
* Run ground cable to GND pin on new ESP32

### RX and TX lines
> This worked for me; other spots may be better

> [!WARNING]
> Lower debug headers are 5v; do not use

- Green (upper) pads are RX to the onboard esp32
    - Solder to TX on standalone esp32
- Yellow (lower) pads are TX to the onbaord esp32
    - Solder to RX on standalone esp32


<img width="2304" height="1042" alt="image" src="https://github.com/user-attachments/assets/9d62bf89-923d-42b1-8cf9-a8a2e2b00e47" />

[Wiring, parking the original, strapping pins](../../docs/installing.md#4-add-your-own-esp32)
in the install guide.

## 5. Assembly

Reverse the disassembly: plug the short wires back in and clip them, slide the
top black control panel back, refit the 4 phillips screws and the front filter
cover.

Reassemble and enjoy! [What to check](../../docs/installing.md#5-assembly)
once it is powered.
