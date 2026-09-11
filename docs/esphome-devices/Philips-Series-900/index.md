---
title: "Philips Series 900 Air Purifier"
date-published: 2026-09-11
type: misc
standard: eu, uk
board: esp32
difficulty: 4
project-url: https://github.com/tuct/levoit/tree/main/devices/philips-900-series
alias:
  - Philips-AC0950
  - Philips-AC0951
---

## Description

> **ESPHome does not control this purifier yet.** The case has been opened, the
> Wi-Fi module identified, the power and UART pads located and an ESP32-C3 fitted
> inside — but the protocol between the module and the purifier's control MCU has
> not been decoded, so there is no working control configuration to publish. This
> page documents the teardown and the capture rig being used to decode it.

Unlike the [Series 600](/devices/philips-series-600/), which carries a locked
ESP32-C3, the Series 900 uses an **MXCHIP EMC6069-P** — a 2.4 GHz Wi-Fi + BLE module
on a small castellated daughterboard, FCC ID
[`P53-EMC6069`](https://fccid.io/P53-EMC6069). It is not an EMW3080 / MX1290, so the
known Realtek RTL8710BN → LibreTiny → ESPHome reflash route does not apply, and
MXCHIP does not publish the module's silicon. The working plan is therefore the same
as on the 600 series: **park the stock module and wire in an ESP32** on the MCU UART.

On stock firmware these units can already be driven locally — without opening the
case — over encrypted CoAP, using the
[ha-philips-airpurifier](https://github.com/ruaan-deysel/ha-philips-airpurifier)
custom component. That remains the working option until the UART protocol is known.

Manufacturer: [Philips](https://www.philips.com) / [Versuni](https://www.versuni.com)

![Philips Series 900 running](./device-running.jpg "Philips Series 900 running")

## Teardown

Twist the top cap counter-clockwise and lift it off — no screws, the same arrangement
as the Series 600. That exposes the control PCB.

`+5V` and `GND` are broken out next to the module, and three further pads at its
top-left edge — labelled A, B and C on the annotated photo in the device guide — are
the candidates for the MCU link. Which of them is TX and which is RX is still being
confirmed.

On the board: the MXCHIP module sits on the right-hand side with series resistors
clustered at its castellated pads, a large unmarked QFP in the centre is the main
MCU, and a connector at the bottom-right is silkscreened `PM2.5`.

> **Unplug the unit first.** Part of the control board is on mains.

## Capturing the MCU UART

The configuration below does not control anything — it listens passively on both
directions of the MCU-to-module link and dumps the frames to the ESPHome log, with
the stock module left in place and running so its conversation can be recorded. A
Seeed XIAO ESP32-C3 fits inside the base alongside the stock module; a logic
analyzer clipped to the same pads works just as well and is what the current
captures were taken with.

Wire an ESP32-C3 read-only: `GND`, one GPIO for the MCU's transmit line and one for
the module's. No transmit line from the ESP.

115200 8N1 is the starting guess, carried over from the Series 600; if the log is
garbage, try 9600 / 38400 / 57600.

```yaml file=config.yaml
```

## Details and instructions

Teardown photos, board notes, the reverse-engineering log and captures:
[tuct/levoit — Philips 900 series](https://github.com/tuct/levoit/tree/main/devices/philips-900-series).

Help decoding the protocol is welcome — captures and findings go in
[Discussions](https://github.com/tuct/levoit/discussions).
