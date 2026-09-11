[← Back](../../README.md)
# Levoit Superior 6000S — Humidifier

> 🚧 **Ported, compiles, not yet run on hardware.** The protocol work comes from
> [Jyers/esphome-projects](https://github.com/Jyers/esphome-projects), ported onto
> this repo's `levoit` component. Nobody here owns a Superior 6000S, so **none of
> it has been confirmed against a real device** — the GPIO pins in particular are
> inherited from the Levoit purifiers and are a guess. Treat this as a starting
> point, and please report back.

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
| MCU link | UART, 115200 8N1 — same framing as the purifiers |
| Fan speeds | **9** (the purifiers have 3–4) |
| Status | 🚧 compiles; unverified on hardware |

## Files

| File | Purpose |
|------|---------|
| [`levoit-superior-6000s.yaml`](./levoit-superior-6000s.yaml) | Flash the device's own ESP32; pulls the component from git |
| [`levoit-superior-6000s_dev.yaml`](./levoit-superior-6000s_dev.yaml) | Same, but builds against this repo's local `components/` |
| [`common.yaml`](./common.yaml) | All the entities — included by both |
| [`secrets-example.yaml`](./secrets-example.yaml) | Copy to `secrets.yaml` and fill in |

## Getting started

1. Copy `secrets-example.yaml` to `secrets.yaml` and fill in your Wi-Fi details.
2. **Check the UART pins.** `rx_pin: GPIO16` / `tx_pin: GPIO17` are carried over
   from the Levoit purifiers and have not been verified on this board. Open the
   unit and confirm before flashing.
3. Flash [`levoit-superior-6000s.yaml`](./levoit-superior-6000s.yaml).

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

**`dry_level` does not act on its own.** There is no standalone dry-level command;
the select just records Low or High, and the value is applied when you pick **Dry**
on the fan entity. Setting the select alone will not start a drying cycle.

## Credits

Protocol reverse-engineering and the original implementation:
**[Jyers](https://github.com/Jyers)** — see
[Jyers/esphome-projects](https://github.com/Jyers/esphome-projects).
