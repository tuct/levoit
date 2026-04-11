

# Project - Free Levoit Air Purifiers - Supported Device List

* [Levoit Core 200s](./levoit-core200s) – Custom ESPHome Firmware 
* [Levoit Core 300s](./levoit-core300s) – Custom ESPHome Firmware
* [Levoit Core 400s](./levoit-core400s) – Custom ESPHome Firmware
* [Levoit Core 600s](./levoit-core600s) – Custom ESPHome Firmware 
* [Levoit Vital 100s](./levoit-vital100s) – Custom ESPHome Firmware
* [Levoit Vital 200S](./levoit-vital200s) – Custom ESPHome Firmware 

Special:
* [Levoit LV PUR 131s](./levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [Levoit Mini](./levoit-mini) – Custom PCB, 3D parts, hardware hack

Core 200S/300S/400S and Vital100/200, only require disassembly to flash firmware (no hardware modifications needed). 
They are using the esphome external component and can be flashed to original ESP32 or easily modded with a new one!
The Mini and LV-PUR 131S require custom hardware hacks and PCB modifications.

## Building Firmware

### Production build (component from GitHub)

Each device folder contains a YAML file per board variant, e.g.:

```
levoit-core300s/levoit-core300s.yaml        # original ESP32-SOLO-1C
levoit-core300s/levoit-core300s-c3.yaml     # ESP32-C3 replacement
levoit-core300s/levoit-core300s-s3.yaml     # ESP32-S3 replacement
```

These pull the `levoit` component directly from GitHub on every compile:

```powershell
esphome compile .\levoit-core300s\levoit-core300s.yaml
```

### Development build (component from local source)

If you are working on the component code locally, use the `_dev` variants instead.
They load the component from `../../components` (the repo root) rather than GitHub:

```powershell
esphome compile .\levoit-core300s\levoit-core300s_dev.yaml
```

The only difference between the two is which source package is included:

| File | Component source |
|------|-----------------|
| `levoit-core300s.yaml` | `packages/source_git.yaml` → GitHub |
| `levoit-core300s_dev.yaml` | `packages/source_local.yaml` → `../../components` |

### Build all dev variants at once

Run the PowerShell script from the `devices/` folder:

```powershell
.\build-all-dev.ps1
```

This compiles every supported device using the local component and prints a pass/fail summary.

### ESPHome web builder / dashboard (no local setup)

Each device folder contains pre-generated `*-builder-*.yaml` files with all configuration inlined — no local clone or package includes needed. Upload directly to the [ESPHome web builder](https://builder.esphome.io) or paste into the ESPHome dashboard.

| File | Board |
|------|-------|
| `levoit-core300s-builder.yaml` | original ESP32-SOLO-1C |
| `levoit-core300s-builder-c3.yaml` | ESP32-C3 replacement |
| `levoit-core300s-builder-s3.yaml` | ESP32-S3 replacement |

(Same pattern applies to all devices.)

To regenerate all builder files after making changes to `common.yaml`:

```powershell
.\make-builder-yaml.ps1
```

---

## Install Methods (ESP32)

Choose the approach that fits your device and risk tolerance:

1) **Reuse Original ESP32 (single ESP)**
  - Put original ESP32 in bootloader, flash ESPHome directly over UART (TX/RX/GND/EN/GPIO0).
  - Simplest wiring; you lose the stock firmware unless you back it up first.

2) **Add Second ESP32 (dual ESP)**
  - Keep the factory ESP32 intact and add a new ESP32 in parallel on UART.
  - Use a 2-position switch on EN to select which ESP32 is active (switch only while powered down).
  - Lets you revert to stock firmware or take MCU updates; recommended for cautious installs.

3) **Replace Module**
  - Desolder/disable the factory ESP32 module and drop in your own (e.g., ESP32-S3/C3).
  - Cleanest for long-term custom firmware; cannot easily revert to stock without rework.

General tips:
- Backup with `esptool read_flash 0 ALL levoit.bin` if possible.
- Common wiring: 3V3, GND, TX→MCU RX, RX→MCU TX, EN, GPIO0 (for boot).
- For WiFi LED/filter LED behaviors and entity list, see the component README.





