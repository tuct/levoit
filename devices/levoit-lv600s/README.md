[← Device list](../../README.md)

# Levoit LV600S / LUH-A602S-WEUR

Local ESPHome support for the ultrasonic warm/cool-mist LV600S, tested on one
EU `LUH-A602S-WEUR` with the original ESP32-SOLO-1C and MCU 1.0.56. See
[Verification](#verification) for what was checked on that unit.

- Module: ESP32-SOLO-1C / ESP32-S0WD, single core, 4 MB on the observed board.
- MCU UART: GPIO17 TX → MCU RX, GPIO16 RX ← MCU TX, **9600 8N1**.
- UART0 for flashing uses GPIO1 TX, GPIO3 RX and common GND. Bridge GPIO0 to
  GND to start the ESP32 ROM bootloader. Use an isolated external 5 V supply
  at the verified board input and a UART adapter with 3.3 V logic. Leave the
  adapter's VCC disconnected. [Disassembly and serial access](HARDWARE.md)
- This example uses ESPHome's default partition table for a **new installation**.
  It is not a drop-in OTA profile for the owner's already converted appliance.
  Existing installations must preserve their own original table and access keys.

Copy `secrets-example.yaml` to a private `secrets.yaml` and replace every
placeholder. Generate the API key with `openssl rand -base64 32` and choose
unique OTA and setup access point passwords.
Build [levoit-lv600s_dev.yaml](levoit-lv600s_dev.yaml) with ESPHome from a local
checkout. This config uses the component in that checkout. The stable Device
Builder catalogue can include LV600S after the component is released.

## Access and recovery

ESPHome resolves the private values at build time. Keep the private
`secrets.yaml` and generated firmware in encrypted storage. The development
example authenticates native OTA with a password, which is distinct from the
API key and the setup Wi-Fi password.

Use the secure owner profile on an already converted appliance that uses its
original partition table and encrypted native OTA. This profile uses one
device-specific key for the API and native OTA, plus separate setup Wi-Fi and
portal upload passwords. Its physical reset clears the stored Wi-Fi login and
settings. The credentials compiled into the firmware survive the reset.

Keep the currently accepted OTA credential, setup Wi-Fi password, any
separate portal upload password, matching recovery image and partition
table in a backup you can open without Home Assistant. Use the current
OTA credential when installing a replacement.
Recover over UART0 if the network paths are unavailable.
[Serial recovery requirements](HARDWARE.md)

## Controls

Power, display, Manual/Auto/Sleep, target RH 40–80%, mist 1–9, warmth 0–3 and
a timer up to 720 minutes. Original Auto and unknown mode are display-only.
The companion Home Assistant integration exposes a native humidifier entity.

The *Target Humidity* number is the target saved on the ESP. Whenever the
appliance is on in Humidity mode and the MCU reports a different target, the
ESP writes the saved one. A target changed on the appliance panel is therefore
not shown here and is replaced at the next power-on or mode change.

**English.** Set mist, warm mist or timer minutes on the ESPHome entities while
the appliance is in standby. The ESP saves them and applies them after a safe,
confirmed power-on. A mist preset selects Manual. A queued warm mist level
waits until the appliance leaves Sleep. The timer starts once and does not
restart on later power cycles after consumption. Setting the timer to `0`
clears the saved preset immediately and requests cancellation when the MCU
confirms that the humidifier is on. A confirmed inactive timer displays `Off`,
as does a standby device that has sent no timer report. Missing or expired
timer data while running displays `Unknown`. A queued duration is shown
separately while the countdown is still off.

**Deutsch.** Nebelstufe, Warmnebelstufe und Timer lassen sich über die
ESPHome-Entitäten im Standby vormerken. Der ESP speichert sie und überträgt
sie nach sicher bestätigtem Einschalten. Eine Nebelvorgabe schaltet auf
**Manuell**. Eine Warmnebelvorgabe wartet, bis der Schlafmodus endet. Der
Timer startet einmal. `0` löscht die Vormerkung sofort. Bestätigt die MCU den
eingeschalteten Zustand, wird der Timerabbruch angefordert. Bei bestätigtem
Timer-Aus und im Standby ohne Timer-Rückmeldung steht `Off`. Fehlen aktuelle Timerdaten,
steht dort `Unknown`. Eine vorgemerkte Dauer erscheint bei weiterhin
ausgeschaltetem Countdown.

| ESPHome entity | Default icon |
|---|---|
| Mist Level / Nebelstufe, Humidifying / Befeuchtet | `mdi:scent` |
| Warm Level / Warmnebelstufe, Heating Enabled / Wärme aktiv | `mdi:heat-wave` |
| Water Tank Empty / Wassermangel | `mdi:water-off` |
| Target Humidity / Zielfeuchte | `mdi:water-percent` |
| Timer Duration / Timer Dauer | `mdi:timer` |
| Timer Remaining / Timer Restzeit | `mdi:progress-clock` |

[Wire format, commands, safety and evidence](PROTOCOL.md).

## Verification

One EU unit, MCU 1.0.56, running this component over the author's
encrypted-OTA builds:

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
captured from this unit inside synthetic frames; the full captures are the
author's and are not in the repository.

Prior work: [wdm230/levoit-lv600s-esphome](https://github.com/wdm230/levoit-lv600s-esphome).
No source from that repository is bundled here. The component belongs to this
repository's MIT-licensed contribution; keep its protocol tests with changes.
