[← Back](../../README.md)

# Levoit Core 300S - Custom Firmware (ESPHome)

ESPHome on the Core 300S, through the `levoit` component: every stock function
in Home Assistant, no cloud. Supported. Adding your own ESP32 is the
recommended route; flashing the original works too. The steps shared by all
devices are in the [install guide](../../docs/installing.md); this page has
what is particular to the Core 300S.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Core 300S |
| Tested MCU FW | 2.0.7, 2.0.11 |
| ESP Module | ESP32-SOLO-1C |
| MCU | SinOne SC95F8616P, LQFP-48: a 1T 8051 core with 64 KB flash (U1, marking in `images/pcb_front.jpg`) |
| Board | CORE300S Ctrl Board V1.4.1 |
| Fan Speeds | 3 |
| CADR (spec) | 214 m³/h |
| Noise | 24–50 dB |
| Room Size | 9–50 m² (97–538 ft²) |
| ESPHome | 2026.6.0+ |
| PM Sensor | PM2008MS |

## Features

| Feature | Type | Notes |
|---------|------|-------|
| Fan | fan | 3 speeds, presets: Manual / Auto / Sleep |
| Auto Mode | select | Default / Quiet / Room Size |
| Auto Mode Room Size | number | 9–50 m² |
| Display | switch | Toggle LED display |
| Child Lock | switch | |
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

![Updated UI in Home Assistant](./images/UpdatedUiInHa.png)

![Core 300S Fan](./images/fan.png)

## Routes onto the device

| Route | Teardown | Notes |
|-------|----------|-------|
| [Add your own ESP32](#4-add-your-own-esp32-recommended) | full | Recommended — keeps the original module intact and reversible |
| [Flash the original ESP32](#3-flash-the-original-esp32) | full + soldering | Smallest parts count, hardest to undo |
| [Over the air](#flash-over-the-air--no-disassembly-third-party) | **none** | Third-party proof of concept, hijacks the stock OTA over the LAN |

## 1. Build the firmware

**With Home Assistant (recommended):** [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha)
→ *Add device* → **Core 300S** → the original ESP, Seeed XIAO ESP32-C3 or
ESP32-S3. [Step by step](../../docs/installing.md#with-home-assistant-recommended).

**Manually:** the files in this folder, built with the ESPHome CLI or pasted
into the Device Builder. [Step by step](../../docs/installing.md#manually).

| Board | ESPHome CLI | Device Builder / web builder |
|-------|-------------|------------------------------|
| original ESP32-SOLO-1C | `levoit-core300s.yaml` | `levoit-core300s-builder.yaml` |
| Seeed XIAO ESP32-C3 | `levoit-core300s-c3.yaml` | `levoit-core300s-builder-c3.yaml` |
| Seeed XIAO ESP32-S3 | `levoit-core300s-s3.yaml` | `levoit-core300s-builder-s3.yaml` |

```bash
esphome run levoit-core300s.yaml
```

## 2. Disassembly

* Place upside down, remove base cover and filter to expose 8 screws (4 have washers)
* Remove all 8 screws — they are soft metal, do not overtighten when reassembling
* Slide a pry tool between the tabs to separate base and top sleeve
* Unplug the logic board

![Teardown](./images/teardown_1.jpg)

![Teardown](./images/teardown_2.jpg)

![Teardown](./images/teardown_3.jpg)

![Teardown](./images/teardown_4.jpg)

## 3. Flash the original ESP32

Solder wires to **TXD0, RXD0, IO0, +3V3, GND** near the ESP32 on the logic
board and connect to a USB-UART converter. On some boards these are
through-holes and soldering may not be needed. Connect **IO0 to GND before
powering on** to enter bootloader mode.

![PCB](./images/pcb_front.jpg)

For flashing you only need to solder to the pin header; the black and white
wires are connected to dump the ESP-to-MCU communication.

Then [back up, flash and, if needed, restore](../../docs/installing.md#3-flash-the-original-esp32):

```bash
esptool read_flash 0 ALL levoit-core300s-backup.bin
esphome run levoit-core300s.yaml
```

> Note: backup may fail on some boards — proceed at your own risk.

Check the [component README](../../components/levoit/README.md) for the UART
pin mapping per board.

## 4. Add your own ESP32 (recommended)

Replacing the original ESP32 allows switching back to original firmware
without re-flashing and makes future updates easier.

**Recommended modules** (compact and reliable):
- [Seeed XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?sensecap_affiliate=3gGMlPv&referring_service=link)
- [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link)

**Wiring:** 4 wires — `+3.3V`, `GND`, `RX`, `TX`

> The RX/TX pads are **not** on the pin header — use the test pads near the original ESP32 on the board.
> Pull the `EN` pin of the original ESP32 to GND to disable it.

![Custom ESP wires](./images/custom_esp_wire.jpg)

**Power:** I connected to 5V, cause i had issues on the 3.3v with my xioa seeed
s3. It rebooted with brownout (power issues). I think my en to gnd connection
might not have been stable but still decided to use 5V.

[Wiring, parking the original, strapping pins](../../docs/installing.md#4-add-your-own-esp32)
in the install guide.

## 5. Assembly

**Placement of new ESP:** the small xiao seeeds fit into the small existing
space, make sure to isolate and solder the connections.

![Custom ESP wires](./images/custom_esp_placement.jpg)

* Plug the logic board back in
* Fit the top sleeve back onto the base
* Refit the 8 screws (the 4 with washers where they came from) — they are soft
  metal, do not overtighten
* Filter and base cover back in

Reassemble and enjoy! [What to check](../../docs/installing.md#5-assembly)
once it is powered.

## Other routes

### Flash Over the Air — No Disassembly (third-party)

[**nico202/levoit-core300s-ota-wifi-flash**](https://github.com/nico202/levoit-core300s-ota-wifi-flash)
gets ESPHome onto the **stock ESP32 without opening the unit or soldering
anything**, by hijacking the device's own firmware update. It is the only route
documented for this model that needs no teardown at all.

It is a **proof of concept by a third party**, not tested by this project. It
worked on a Core 300S EU with stock `mainFw` 1.1.08, upgraded to 2.0.00.

#### Why it works

* The purifier is told by the VeSync cloud to fetch an update, and then makes a
  **plain HTTP GET** for the firmware
* A VeSync OTA file is a **128-byte header** (MD5, name, version, region)
  followed by a raw ESP32 application image
* There is **no firmware signature and Secure Boot is disabled**, so an image
  wrapped in that container is accepted

#### How it goes

1. Build an ESPHome image and wrap it in the VeSync OTA container
2. Serve it from a Linux box on the same subnet as the purifier
3. Redirect the device's HTTP firmware fetch to that server, and block its HTTPS
   to the vendor's firmware host so it cannot get the genuine binary. The device
   stays connected to the real cloud throughout
4. Trigger an update from the VeSync app or API
5. The purifier reboots into the ESPHome fallback access point; enter Wi-Fi
   credentials, and from then on updates are ordinary ESPHome OTA

#### What you need

* A Linux machine with root on the same network — a Raspberry Pi is suggested
* Python 3, `iptables`, and `scapy` or `dsniff`/`arpspoof`
* ESPHome, via pip or Docker
* A VeSync account with the device added; `pyvesync` optionally, for CLI cloud
  queries

The same method works on the **Core 400S** (confirmed by
[@iNDi](https://github.com/tuct/levoit/issues/76)); see the
[Core 400S guide](../levoit-core400s/README.md#flash-over-the-air--no-disassembly-third-party).

#### Caveats

* **The version string in the header must match exactly** what the cloud
  announces, or the device aborts partway through the update
* **The header layout can vary by model and firmware.** The region field is at
  offset 77 here; on a Core 400S [@iNDi](https://github.com/tuct/levoit/issues/76)
  found it at **offset 76**. Check against your own OTA capture
* **Intercepting network traffic you do not own may be illegal where you live.**
  Only do this to your own hardware
* It may void your warranty, and the author offers no warranty of their own
* A proof of concept — if it goes wrong mid-flash you are back to the soldered
  routes above to recover

MIT licensed. Credits acvigue, mulcmu, tuct, epicRE, and the pyvesync and
ESPHome communities.

**Not part of this project**, and not built into it — it stays a third-party
tool you run yourself, at your own risk. Once ESPHome is flashed, the device is
ordinary: `esphome run` / `upload` work over Wi-Fi, and the
[Levoit ESPHome Home Assistant integration](https://github.com/tuct/levoit-esphome-ha)
keeps it updated over the air.

## Related — the non-smart Core 300 variants

This guide covers the **Core 300S**, the Wi-Fi model with an ESP32 and a control MCU
on a UART link. The plain **Core 300** and **Core 300-P** have no Wi-Fi and no MCU
link, so the `levoit` component does not apply to them — they need the controller
replaced instead:

| Variant | Route | Link |
|---------|-------|------|
| Core 300-P | Ready-made ESP32-C6 replacement controller running ESPHome. Reuses the stock fan controller and power supply; adds 4-speed fan control, 14 WS2815B RGB LEDs and 3 spare GPIOs for air-quality or VOC add-ons. **Check the Intertek model number on the back (5014566 vs 5030453) before ordering** — the wrong variant may need the cable connections re-soldered. | [Silo City Labs ↗](https://shop.silocitylabs.com/products/core300-p) |
| Core 300 | DIY: an ESP32 wired straight to the fan-speed lines, exposed as 3 interlocked GPIO switches. No MCU, no UART, no sensors. | [Reddit write-up ↗](https://www.reddit.com/r/homeassistant/comments/1rqz9gq/turned_a_broken_dumb_air_purifier_into_a_smart/) |
