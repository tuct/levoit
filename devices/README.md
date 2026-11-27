

# Project - Free Levoit Air Purifiers - Supported Device List

* [Levoit Core 200s](./levoit-core200s) – Custom ESPHome Firmware 
* [Levoit Core 300s](./levoit-core300s) – Custom ESPHome Firmware
* [Levoit Core 400s](./levoit-core400s) – Custom ESPHome Firmware
* [Levoit Core 600s](./levoit-core600s) – Custom ESPHome Firmware 
* [Levoit Vital 100s](./levoit-vital100s) – Custom ESPHome Firmware
* [Levoit Vital 200S](./levoit-vital200s) – Custom ESPHome Firmware 

Special:
* [Levoit LV PUR 131s](./levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [Levoit LV PUR 131](./levoit-lv131/) – Custom Firmware + temperature sensor + MCU & sensor upgrade + hardware hack
* [Levoit Mini](./levoit-mini) – Custom PCB, 3D parts, hardware hack

Core 200S/300S/400S and Vital100/200, only require disassembly to flash firmware (no hardware modifications needed). 
They are using the esphome external component and can be flashed to original ESP32 or easily modded with a new one!
The Mini, LV-PUR 131, and LV-PUR 131S require custom hardware hacks and PCB modifications.

## Building Firmware

### Production build (component from GitHub)

Each device folder contains a YAML file per board variant, e.g.:

```
levoit-core300s/levoit-core300s.yaml        # original ESP32-SOLO-1C
levoit-core300s/levoit-core300s-c3.yaml     # ESP32-C3 replacement
levoit-core300s/levoit-core300s-s3.yaml     # Seeed XIAO ESP32-S3 replacement
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

Each device folder contains pre-generated `*-builder*.yaml` files with all configuration inlined — no local clone or package includes needed. Upload directly to the [ESPHome web builder](https://builder.esphome.io) or paste into the ESPHome Device Builder.

| File | Board |
|------|-------|
| `levoit-core300s-builder.yaml` | original ESP32-SOLO-1C |
| `levoit-core300s-builder-c3.yaml` | Seeed XIAO ESP32-C3 replacement |
| `levoit-core300s-builder-s3.yaml` | Seeed XIAO ESP32-S3 replacement |

(Same pattern for all devices; a file whose name does not start with the folder name, such as `philips-ac0650.yaml`, becomes `philips-ac0650-builder.yaml`.)

They read `wifi_ssid`, `wifi_password`, `wifi_ap_password`, `api_key` and `ota_password` from your `secrets.yaml`, and pull the component from the **`stable`** branch, which moves to every release, with `refresh: always` so every build fetches it (ESPHome's default is once a day, which would rebuild an old commit right after a release). For every merge instead, change `ref: stable` to `ref: main` in your copy.

**Don't edit them in the repo.** They are generated from the device yaml and its `common.yaml`; edit those and regenerate (Windows PowerShell 5.1 or PowerShell 7):

```powershell
.\make-builder-yaml.ps1           # regenerate all
.\make-builder-yaml.ps1 -Check    # what CI runs: fails if one is out of date
```

CI (`Builder files` workflow) runs the check and compiles every builder file against the components of the PR.

The script also writes [`templates.json`](./templates.json): the builder files the [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha) integration offers when adding a device (variant `original`, `xiao_c3` or `c3`, with model, title, board and default name; S3 builder files are not offered). It is generated and checked like the builder files; add a title for a new device folder in the script's `$Titles`. The Sprout is the exception: its full config needs the local `levoit_audio` component, so `levoit-sprout-builder.yaml` is kept by hand (no sound).

[`onboarding.json`](./onboarding.json) is what the integration's *Add device* page shows per device: a photo from [`docs/images/devices/`](../docs/images/devices/), the status, spec and a few words from the [README](../README.md) gallery, how easy it is to open, and the routes (`flash` the original ESP, `add_esp`, `custom` hardware). Devices without a template are listed there with a link to their guide. It is kept by hand: when a device, its photo or its gallery card changes, change its entry too. The integration reads it from `main`, so an edit shows up without a release of either. CI checks it (`.github/scripts/check_onboarding.py`): known fields and values only, every photo and guide exists, every folder with a template has an entry.

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
- **Look at the board before opening it.** Levoit's FCC filings are under
  grantee [`2ARBY`](https://fccid.io/2ARBY) (Arovast Corporation) and most
  include internal photos of the bare PCB — enough to spot the Wi-Fi module
  and the test pads. Examples:
  [Core 200S](https://fccid.io/2ARBY-CORE-200S) ·
  [Core 300S](https://fccid.io/2ARBY-CORE-300S) ·
  [Core 600S](https://fccid.io/2ARBY-CORE600S) ·
  [Sprout](https://fccid.io/2ARBY-B381S). Note the hyphenation is
  inconsistent (`CORE-300S` but `CORE600S`), so browse the grantee index
  rather than guessing an ID.
- Backup with `esptool read_flash 0 ALL levoit.bin` if possible.
- Common wiring: 3V3, GND, TX→MCU RX, RX→MCU TX, EN, GPIO0 (for boot).
- For WiFi LED/filter LED behaviors and entity list, see the component README.





