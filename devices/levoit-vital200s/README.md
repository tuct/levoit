[← Back](../../README.md)

# Levoit Vital 200S (Pro) - Custom Firmware (ESPHome)

Same protocol as the Vital 100S. Tested with original ESP by @dnsefe.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Vital 200S / Vital 200S Pro |
| Tested MCU FW | 1.0.5 |
| ESP Module | ESP32-C3-SOLO-1 |
| Board | TODO |
| Fan Speeds | 4 |
| CADR (spec) | 415 m³/h |
| Room Size | 9–87 m² (97–936 ft²) |
| ESPHome | 2026.1.2+ |

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

## Teardown / Disassembly

> TODO: add teardown steps and photos

## Debug Header Pinout

> TODO: add pinout and photos

## Wiring New ESP

> TODO: add wiring photos and pin mapping

## Flash

### Prerequisites

Connect to the debug header with a USB-UART adapter (3.3V TTL), crossing TX/RX:
- Adapter TX → MCU RX
- Adapter RX → MCU TX

Connect **IO0 to GND before powering on** to enter bootloader mode.

### Backup Existing Firmware

```bash
esptool read_flash 0 ALL levoit-vital200s-backup.bin
```

> Note: may fail if watchdog-protected. Try while powered externally.

### Configure

1. Copy `secrets-example.yaml` → `secrets.yaml` and fill in your Wi-Fi and encryption key
2. Adjust the device name in the config if running multiple units
3. Check the [component README](../../components/levoit/README.md) for UART pin mapping per board

### Flash

```bash
esphome run levoit-vital200s-c3.yaml
```

Reassemble and enjoy!

### ESPHome Web Builder / Dashboard

Use the pre-generated builder yaml to flash without a local clone — all config is inlined, no `!include` or packages needed:

| File | Board |
|------|-------|
| `levoit-vital200s-builder.yaml` | original ESP32-C3-SOLO-1 |
| `levoit-vital200s-builder-c3.yaml` | ESP32-C3 replacement |
| `levoit-vital200s-builder-s3.yaml` | ESP32-S3 replacement |

Upload to the [ESPHome web builder](https://builder.esphome.io) or paste into the ESPHome dashboard. Regenerate with `.\make-builder-yaml.ps1` from the `devices/` folder.

### Restore Original Firmware

```bash
esptool erase_flash
esptool write_flash 0x00 levoit-vital200s-backup.bin
```
