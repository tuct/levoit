[← Back](../../README.md)

# Levoit LV600S / LUH-A602S-WEUR

Local ESPHome support for the ultrasonic warm/cool-mist LV600S, tested on one
EU `LUH-A602S-WEUR` with the original ESP32-SOLO-1C and MCU 1.0.56. See
[Verification](#verification) for what was checked on that unit.
Flashing the original ESP32 is the route documented for this model. The steps
shared by all devices are in the [install guide](../../docs/installing.md);
this page has what is particular to the LV600S.

## Quick Facts

| Item | Value |
|------|-------|
| Model | LV600S, EU `LUH-A602S-WEUR` |
| Tested MCU FW | 1.0.56 |
| Module | ESP32-SOLO-1C / ESP32-S0WD, single core, 4 MB on the observed board. |
| MCU UART | GPIO17 TX → MCU RX, GPIO16 RX ← MCU TX, **9600 8N1**. |
| Component | `levoit` with `model: LV600S` (shared model), or the standalone [`lv600s_humidifier`](../../components/lv600s_humidifier/README.md) |

## Features

Power, display, Manual/Auto/Sleep, target RH 40–80%, mist 1–9, warmth 0–3 and
a timer up to 720 minutes. Original Auto and unknown mode are display-only.
The shared model exposes one ESPHome fan for power, nine mist levels, and the
presets Manual, Sleep and Humidity. Selecting a level wakes the appliance if
needed and selects Manual. The component waits for a safe, confirmed power-on
before sending the selection. It publishes the state the MCU reports.

The *Target Humidity Preset* number is saved on the ESP and applied in
Humidity mode. Warm mist and a timer entered while the device is in standby
are saved and applied after confirmed power-on. The standalone configuration
also offers a separate mist level number to save a standby preset.
A queued warm mist level waits until the appliance leaves Sleep. A confirmed timer start consumes
its preset. If the write times out, the ESP keeps the preset for the next
power-on. Setting the timer to `0`
clears the saved preset immediately and requests cancellation when the MCU
confirms that the humidifier is on. A confirmed inactive timer displays `Off`,
as does a standby device that has sent no timer report. Missing or expired
timer data while running displays `Unknown`. A queued duration is shown
separately while the countdown is still off.

| ESPHome entity | Default icon |
|---|---|
| Mist Level, Humidifying | `mdi:scent` |
| Warm Level, Heating Enabled | `mdi:heat-wave` |
| Water Tank Empty | `mdi:water-off` |
| Target Humidity | `mdi:water-percent` |
| Timer Duration | `mdi:timer` |
| Timer Remaining | `mdi:progress-clock` |

## Verification

One EU unit, MCU 1.0.56, running the standalone component over encrypted OTA:

- Power, display, Manual/Humidity/Sleep, target humidity, mist 1–9 and
  warmth 0–3, each confirmed by the MCU's following status report.
- Tank removed, empty tank installed, refilled tank, and water running out
  while on (the MCU goes to standby about 10 s later by itself). With an empty
  tank the component rejected power-on, mist, warmth, mode, timer and display
  writes and confirmed an explicit off.
- Timer start and cancel, a one-minute timer running to expiry with automatic
  power-off, and standby presets for mist, warmth and timer applied after
  power-on.
- Pairing and reset events from the physical power button.
- Sleep entered from Manual with no configured target, which the MCU reports
  as target 100; an earlier candidate timed out on that.

Not covered: the stock ESP firmware's power-up handshake with the MCU, other
board or MCU revisions, long-term operation, and the heater's actual output
(warmth is the commanded level, not a temperature). Original Auto is read-only.
The native tests in `components/lv600s_humidifier/tests` replay payloads
captured from this unit inside synthetic frames. The shared-core tests also
replay the [published UART frames](uart/README.md), exercise the model's
confirmed write path and cover the timer and loss of live status.
The shared model was also tested on the assembled EU unit with MCU 1.0.56.
The owner checked the display, mist, physical buttons and tank. Automated
checks read the fan, mode, target humidity, timer and tank states during
those tests. See the [hardware verification checklist](SHARED-CORE-TEST.md).

## 1. Build the firmware

**With Home Assistant (recommended):** No Home Assistant template yet: build it manually.

**Manually:** the files in this folder, built with the ESPHome CLI from a
local checkout. [Step by step](../../docs/installing.md#manually).

| File | Use |
|------|-----|
| [`levoit-lv600s-shared_dev.yaml`](levoit-lv600s-shared_dev.yaml) | `levoit:` with `model: LV600S`, the shared model; includes `network.yaml` and `common-shared.yaml` |
| [`levoit-lv600s_dev.yaml`](levoit-lv600s_dev.yaml) | The standalone `lv600s_humidifier` component; includes `common.yaml` |
| `lv600s-controls.yaml` | The controls, included by `common-shared.yaml` and `common.yaml` |
| `network.yaml` | Logger, API, OTA and Wi-Fi, included by both |
| `secrets-example.yaml` | Copy to `secrets.yaml` and fill in |

There are no Device Builder files for this model yet.

Generate the API key with `openssl rand -base64 32` and choose
unique OTA and setup access point passwords.
Build [levoit-lv600s-shared_dev.yaml](levoit-lv600s-shared_dev.yaml) with ESPHome
from a local checkout to use `levoit:` with `model: LV600S`. The
[standalone development YAML](levoit-lv600s_dev.yaml) remains available for
existing installations. Both use the checkout's component sources. The
shared model uses a development configuration until it ships in a stable
component release.

- This example uses ESPHome's default partition table for a **new installation**.
  For an OTA migration, preserve the existing partition table and access keys.

## 2. Disassembly

The steps, photos and safety warning are in
[LV600S EU teardown and serial access](HARDWARE.md#disassembly). Unplug the
mains cord first, and empty and dry the appliance before opening it.

## 3. Flash the original ESP32

- UART0 for flashing uses GPIO1 TX, GPIO3 RX and common GND. Bridge GPIO0 to
  GND to start the ESP32 ROM bootloader. Use an isolated external 5 V supply
  at the verified board input and a UART adapter with 3.3 V logic. Leave the
  adapter's VCC disconnected. [Disassembly and serial access](HARDWARE.md)

The pads, the 5 V input and the first backup read are in
[Power and first read](HARDWARE.md#power-and-first-read); the generic
procedure is in the [install guide](../../docs/installing.md#3-flash-the-original-esp32).

### Access and recovery

ESPHome resolves the private values at build time. Keep the private
`secrets.yaml` and generated firmware in encrypted storage. The development
example authenticates native OTA with a password, which is distinct from the
API key and the setup Wi-Fi password.

For an existing installation with encrypted native OTA, use its current OTA
credential and preserve the device's configured access settings. A physical
reset with the recovery profile clears the stored Wi-Fi login and settings.
The credentials compiled into the firmware survive the reset.

Keep the currently accepted OTA credential, setup Wi-Fi password, any
separate portal upload password, matching recovery image and partition
table in a backup you can open without Home Assistant. Use the current
OTA credential when installing a replacement.
Recover over UART0 if the network paths are unavailable.
[Serial recovery requirements](HARDWARE.md)

## 4. Add your own ESP32 (recommended)

Not documented for this model yet. The MCU link is in [Quick Facts](#quick-facts);
the generic steps are in the [install guide](../../docs/installing.md#4-add-your-own-esp32).

## 5. Assembly

The reassembly notes are in
[What the first conversion established](HARDWARE.md#what-the-first-conversion-established).
Reconnect mains only with the housing closed.
[What to check](../../docs/installing.md#5-assembly) once it is powered.

## Hardware & protocol notes

[Wire format, commands, safety and evidence](PROTOCOL.md).
Teardown and serial access: [HARDWARE.md](HARDWARE.md).

## Credits

Prior work: [wdm230/levoit-lv600s-esphome](https://github.com/wdm230/levoit-lv600s-esphome).
No source from that repository is bundled here. The component belongs to this
repository's MIT-licensed contribution; keep its protocol tests with changes.
