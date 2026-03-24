[← Back to Free Levoit Project](../README.md)

# Levoit Core 600S - Custom Firmware (ESPHome)

See [Levoit Component](../../../components/levoit/README.md) for complete component documentation.

## Quick Facts

| Item | Value |
| --- | --- |
| Model | Core 600S |
| Tested MCU FW | 2.0.01 |
| ESP | unknown (replace with custom ESP32) |
| Speeds | 4 levels |
| CADR (spec) | 641 m³/h |
| ESPHome | 2025.12.5+ |
| Entities | Fan (manual/auto/sleep), PM2.5, AQI, Current CADR, Filter Life Left, Light Detect, Display, Child Lock, Timer, Auto Mode, Filter Low (binary), Reset Filter Stats (button) |

## Features

* Fan component with modes (Manual, Auto, Sleep)
* PM2.5 sensor and AQI
* Light Detect switch (auto-dims display in dark rooms)
* Display on/off switch
* Child Lock switch
* Auto Mode select (Default / Quiet / Efficient with room size)
* Timer (minutes)
* Current CADR sensor (m³/h), Filter Life Left (%) sensor
* Filter Low binary sensor (<5%)
* Reset Filter Stats button (resets CADR/runtime counters)
* Filter lifetime configurable (months), tracked from runtime and speed

## Protocol Notes

Same UART protocol as Core 300S/400S with these differences:
- Status push: `CMD=01 40 41` (vs `B0 40` / `30 40` on 300S/400S)
- Light Detect command: `CMD=01 E9 A5` PAY=`01`/`00`
- Status byte layout shifted — display at `[8]`, AQI at `[11]`, PM2.5 at `[12-13]`, child lock at `[14]`, auto mode at `[15]`, efficiency area at `[16-17]`, light detect at `[21]`

## Flash

Rename `secrets-example.yaml` to `secrets.yaml` and set your WiFi and encryption key.

Use the C3 or S3 config depending on which replacement ESP module you are using:

```bash
esphome run levoit-core600s-c3.yaml
```

```bash
esphome run levoit-core600s-s3.yaml
```

### Backup Existing Firmware

```bash
esptool read_flash 0 ALL levoit-core600s-backup.bin
```

### Restore Original Firmware (if needed)

```bash
esptool erase_flash
esptool write_flash 0x00 levoit-core600s-backup.bin
```
