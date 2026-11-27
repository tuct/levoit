[← Back](../../README.md)
# Levoit Superior 6000S — Humidifier

> 🚧 **Early support.** The protocol work comes from
> [Jyers/esphome-projects](https://github.com/Jyers/esphome-projects), ported onto
> this repo's `levoit` component. The ESP module and its MCU link pins are
> confirmed on an **LEH-S601S-WUS** by egoleos in
> [discussion #78](https://github.com/tuct/levoit/discussions/78), with initial
> working tests: an ESP32-C3-SOLO-1, link on GPIO18 / GPIO19. The entities are
> not all verified yet, so please report back.

The Superior 6000S is an evaporative humidifier, but it speaks the **same MCU
protocol** as the Levoit air purifiers, so it needs no new component — just
`model: SUPERIOR6000S` on the existing
[`levoit`](../../components/levoit/README.md) component.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Superior 6000S |
| Brand | Levoit (VeSync) |
| Type | Evaporative humidifier |
| Tank | 6 L |
| Component | [`levoit`](../../components/levoit/README.md), `model: SUPERIOR6000S` |
| ESP module | Espressif ESP32-C3-SOLO-1 (LEH-S601S-WUS, [#78](https://github.com/tuct/levoit/discussions/78)) |
| MCU link | UART, 115200 8N1 — same framing as the purifiers; module pins 27 (IO18, ESP RX) / 28 (IO19, ESP TX) |
| Fan speeds | **9** (the purifiers have 3–4) |
| Status | 🚧 pins confirmed, initial tests working ([#78](https://github.com/tuct/levoit/discussions/78)) |

## Files

| File | Purpose |
|------|---------|
| [`levoit-superior-6000s.yaml`](./levoit-superior-6000s.yaml) | Flash the device's own ESP32-C3-SOLO-1; pulls the component from git |
| [`levoit-superior-6000s_dev.yaml`](./levoit-superior-6000s_dev.yaml) | Same, but builds against this repo's local `components/` |
| [`levoit-superior-6000s-builder.yaml`](./levoit-superior-6000s-builder.yaml) | Everything inlined, for the ESPHome Device Builder (generated, see [devices/README.md](../README.md#esphome-web-builder--dashboard-no-local-setup)) |
| [`levoit-superior-6000s-c3.yaml`](./levoit-superior-6000s-c3.yaml) / [`_dev`](./levoit-superior-6000s-c3_dev.yaml) / [`builder-c3`](./levoit-superior-6000s-builder-c3.yaml) | A replacement Seeed XIAO ESP32-C3 on the MCU link (RX GPIO5 = pad D3 to module pin 27, TX GPIO4 = pad D2 to pin 28, as on the working Eleva install); untested |
| [`levoit-superior-6000s-s3.yaml`](./levoit-superior-6000s-s3.yaml) / [`_dev`](./levoit-superior-6000s-s3_dev.yaml) / [`builder-s3`](./levoit-superior-6000s-builder-s3.yaml) | A replacement Seeed XIAO ESP32-S3 on the MCU link (RX GPIO5 = pad D4 to module pin 27, TX GPIO4 = pad D3 to pin 28, as confirmed for the LEH-S601S-WUS in [#78](https://github.com/tuct/levoit/discussions/78)); untested with an S3 |
| [`common.yaml`](./common.yaml) | All the entities — included by both |
| [`secrets-example.yaml`](./secrets-example.yaml) | Copy to `secrets.yaml` and fill in |

## Getting started

1. Copy `secrets-example.yaml` to `secrets.yaml` and fill in your Wi-Fi details.
2. **Back up the stock firmware** (`esptool read-flash 0 ALL backup.bin`).
3. Flash [`levoit-superior-6000s.yaml`](./levoit-superior-6000s.yaml) over the
   module's **programming UART pads** (a USB-UART adapter: its TX to the
   module's RX, its RX to the module's TX, 3.3 V). Hold **GPIO9** low during
   reset for download mode; the pad marked *IO0* is GPIO9 on the C3. Release
   it and power-cycle after flashing.
4. The MCU link (GPIO18 / GPIO19) is separate from the programming pads and
   needs no extra wiring on the stock module. The config logs on UART0 (the
   programming pads); ESPHome warns that GPIO18/19 are the USB pins, which is
   expected.

To keep the stock module instead, wire your own ESP to the MCU link and use the
`-c3` or `-s3` config from [Files](#files).

**Recommended modules:**
- [Seeed XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?sensecap_affiliate=3gGMlPv&referring_service=link)
- [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link)

> [!TIP]
> No YAML to write: in Home Assistant, the [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha) integration's
> *Add device* makes a ready-to-flash config for either board from this model's
> template. You only add a name and your Wi-Fi.

## Entities

| Platform | `type` | Notes |
|----------|--------|-------|
| `fan` | — | on/off, speeds **1–9**, modes Manual / Sleep / Auto / Humidity / Dry |
| `number` | `humidity_target` | target relative humidity (%) |
| `number` | `timer` | **hours**, 0 = off (see the note below) |
| `number` | `filter_lifetime_months` | filter interval |
| `sensor` | `humidity` | measured RH (%) |
| `sensor` | `temperature` | measured temperature (°C) |
| `sensor` | `filter_life_mcu` | filter life as the MCU reports it |
| `sensor` | `filter_life_left` | filter life derived from runtime |
| `sensor` | `timer_current` | hours remaining |
| `select` | `auto_profile` | Home / Away |
| `select` | `humidity_subtype` | Smart / Fan |
| `select` | `dry_level` | Low / High — see the note below |
| `switch` | `display` · `child_lock` | |
| `switch` | `auto_dry_power_off` | run a drying cycle after power off |
| `switch` | `auto_dry_water_empty` | run a drying cycle when the tank runs dry |
| `binary_sensor` | `water_tank_empty` · `humidifying` · `dry_active` | |
| `binary_sensor` | `cover_open` · `filter_low` | |
| `button` | `reset_filter_stats` | |
| `text_sensor` | `mcu_version` · `esp_version` | |

## Two things that behave differently from the purifiers

**The timer runs on the ESP, not the MCU.** On this device the MCU does not count
down on its own — it only stores a "remaining" value that gets pushed to it. The
component therefore owns the countdown and refreshes the MCU once a minute, and
when it reaches zero it sends the zero a few times (a single one is sometimes
missed) before switching the device off. The `timer` number is in **hours** here,
where the purifiers use minutes.

**`dry_level`** changes Low/High immediately while drying. Otherwise it remembers
the choice for the next **Dry** preset request; inactive MCU reports do not erase
that choice. On MCU 1.0.9 the level command also starts drying, so selecting a
level while inactive does not send it ([capture](uart/dry_controls.txt)).

## Credits

Protocol reverse-engineering and the original implementation:
**[Jyers](https://github.com/Jyers)** — see
[Jyers/esphome-projects](https://github.com/Jyers/esphome-projects).
