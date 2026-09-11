[← Back to Components](../README.md)

# Philips / MUJI ESPHome Component

Custom ESPHome component for **Philips (Versuni) air purifiers**, including those sold under the MUJI brand — the **600 Series** (`AC0650/10`, `AC0651/10`) and the **900 Series** (`AC0950`, `AC0951`) — enabling local control without cloud dependency.

It works much like the [Levoit component](../levoit/README.md), just speaking Philips' own `FE FF` binary UART protocol instead of the Levoit TLV one.

[See the Supported Models overview](../../README.md) · [Full protocol reverse-engineering notes](../../devices/philips-600-series/README.md)

## Supported Models

| Model | Notes |
|-------|-------|
| `AC0650/10` | Fan (Sleep / Medium / Turbo), filters (pre-filter + HEPA) |
| `AC0651/10` | Above **+ PM2.5 (PM1003), allergen / AQI index, Auto fan mode, standby sensor monitoring** |
| `AC0950` | 600-series feature set **+ child lock, beep, display brightness, sleep timer** |
| `AC0951` | Above **+ PM2.5, allergen / AQI index, Auto fan mode, standby sensor monitoring** |

> The 900 series speaks the same protocol with the same datapoint numbering, so
> support is mostly shared. Two wire values differ — medium fan mode is `0x13`
> (not `0x01`) and the HEPA total is 9600 (not 4800) — and it adds four controls
> the 600 series does not have. Both are handled by `model:`.
>
> Protocol details and the captures behind them:
> [600 series](../../devices/philips-600-series/README.md) ·
> [900 series](../../devices/philips-900-series/README.md).
>
> ✅ **An AC0951 is confirmed running this on hardware.** The `AC0950` is not:
> every capture and the working install are from an AC0951, so its support is an
> assumption modelled on the AC0650/AC0651 split.

Both speak the identical protocol; the component selects model-specific behaviour via the `model:` option.

## Installation

### Hardware Setup

> 🔒 **Secure boot is enforced** on the stock ESP32-C3 module, so custom firmware **cannot** be flashed onto it. The approach is to **add your own ESP32** and disable the original module.

1. Disable the stock module: pull its `EN` pin to **GND** (a ~10 kΩ resistor to GND, optionally a small cap). It already has a 10 kΩ pull-up, so this draws only a fraction of a mA.
2. Wire a new ESP32 (e.g. **XIAO Seeed ESP32-C3**) to the MCU↔module UART:
   - Power (3.3 V) and GND from the purifier PCB
   - **ESP TX → MCU RX**, **ESP RX → MCU TX**
3. The link runs at **115200 baud, 8N1**.

> On the ESP32-C3 the default logger UART (UART0) shares GPIO20/21 with the MCU link, so log over the native USB instead: `logger: { hardware_uart: USB_SERIAL_JTAG }`.

See the [device guide](../../devices/philips-600-series/README.md) for teardown, pin locations, and protocol details.

## Configuration

```yaml
external_components:
  - source:
      type: local
      path: ../../components
    components: [philips]

uart:
  - id: uart_mcu
    tx_pin: GPIO21   # ESP TX → MCU RX
    rx_pin: GPIO20   # MCU TX → ESP RX
    baud_rate: 115200

philips:
  id: purifier
  model: AC0651      # AC0650 (base) or AC0651 (PM2.5 + Auto)

fan:
  - platform: philips
    philips: purifier
    name: "Fan"        # speeds: 1=Sleep, 2=Medium, 3=Turbo; Auto preset on AC0651

sensor:
  - platform: philips
    philips: purifier
    name: "Pre-filter"
    type: filter_clean
  - platform: philips
    philips: purifier
    name: "HEPA Filter"
    type: filter_lifetime
  # --- AC0651 only ---
  - platform: philips
    philips: purifier
    name: "PM2.5"
    type: pm2_5
  - platform: philips
    philips: purifier
    name: "Allergen Index"
    type: allergen_index

button:
  - platform: philips
    philips: purifier
    name: "Reset Pre-filter"
    type: reset_prefilter
  - platform: philips
    philips: purifier
    name: "Reset HEPA Filter"
    type: reset_hepa

switch:
  # --- AC0651 only ---
  - platform: philips
    philips: purifier
    name: "Standby Sensor Monitoring"
    type: standby_sensor

text_sensor:
  - platform: philips
    philips: purifier
    name: "MCU Version"
    type: mcu_version
```

Ready-made device configs live in [`devices/philips-600-series/`](../../devices/philips-600-series/).

## Entities

| Platform | `type` | Models | Description |
|----------|--------|--------|-------------|
| `fan` | — | both | On/off = power; speed 1/2/3 = Sleep/Medium/Turbo; **Auto** preset on AC0651 |
| `sensor` | `filter_clean` | both | Pre-filter remaining (%) |
| `sensor` | `filter_lifetime` | both | HEPA filter remaining (%) |
| `sensor` | `pm2_5` | AC0651 | PM2.5 in µg/m³ (PM1003 sensor) |
| `sensor` | `allergen_index` | AC0651 | Allergen / AQI index (1–12) |
| `button` | `reset_prefilter` | both | Reset the pre-filter counter |
| `button` | `reset_hepa` | both | Reset the HEPA filter counter |
| `switch` | `standby_sensor` | AC0651 / AC0951 | Keep the PM sensor monitoring while on standby |
| `switch` | `child_lock` | 900 only | Lock the panel buttons |
| `switch` | `beep` | 900 only | Audible feedback (written as 0 / 100, not 0 / 1) |
| `select` | `display_brightness` | 900 only | `off` / `low` / `bright` |
| `number` | `timer` | 900 only | Sleep timer in hours, 0 (off) to 12 |
| `sensor` | `timer_remaining` | 900 only | Minutes left on the sleep timer (counts down; read-only) |
| `text_sensor` | `mcu_version` | both | MCU firmware version (from the boot device-info report) |

## Notes

- The MCU protocol is strict **request → response**. The component is lock-step (one frame out, wait for the reply, small inter-frame gap) like the stock module — async writes would collide with the MCU's status traffic and get dropped.
- It handles the boot handshake, polls operating state (~1 s) and filters (~10 s), and reports Wi-Fi "connected" to the front panel.
- Model-specific entities are simply omitted from configs that do not have them; the component gates the Auto preset and the PM/allergen/standby datapoints by `model:`.
- **Display brightness reads `off` whenever the unit is powered off**, because the MCU zeroes that datapoint with the power. Read it together with the fan's on/off state rather than treating `off` as a user setting.
- The sleep timer writes whole hours; the minutes remaining are counted down by the MCU and surface as the separate read-only `timer_remaining` sensor.
- Physical button presses never produce a write on the bus — the MCU reports them only in its status frames, which the component polls once a second.

**Requires:** ESPHome 2026.05.3+
