[← Back](../../README.md)
# Philips Series 900 Air Purifier — AC0950 / AC0951

> 🔬 **Research folder — no working ESPHome support yet.** The board has been
> opened and the Wi-Fi module identified, but the MCU↔module protocol has not
> been decoded. The only thing here you can flash today is
> [`philips-900-uart-sniffer.yaml`](./philips-900-uart-sniffer.yaml), which
> passively captures that link so it *can* be decoded. Everything else in this
> folder is a WIP skeleton.

The goal for these units is the same move as the
[600-series](../philips-600-series): **disable the stock Wi-Fi module and drive
the purifier's MCU from your own ESP32** over the internal UART — fully local,
no cloud, no vendor firmware left in the loop.

Until that works, an AC0950/AC0951 on stock firmware can already be run
cloud-free over local CoAP — see
[Cloud-free without ESPHome](../../README.md#cloud-free-without-esphome--philips--versuni-over-local-coap)
in the root README.

## Quick Facts

| Item | Value |
|------|-------|
| Models | AC0950, AC0951 |
| Brand | Philips (Versuni) |
| Stock Wi-Fi module | **MXCHIP EMC6069-P** — 2.4 GHz Wi-Fi + BLE, FCC ID [`P53-EMC6069`](https://fccid.io/P53-EMC6069) |
| Module form | Blue castellated daughterboard (`EMC6069-HF`), soldered to the main PCB |
| Main MCU | Unmarked QFP, centre of the board — not yet identified |
| MCU link | UART assumed, pins/baud/protocol **all unverified** |
| ESPHome support | ❌ none yet — `philips` component knows only AC0650/AC0651 |
| Stock local control | ✅ CoAP (`AWS_Philips_AIR`) without opening the case |

## Opening the unit

> ⚠️ **Unplug it first.** Part of the control board is on mains.

No screws to start with: **twist the top cap counter-clockwise** (to the left)
and lift it off — the same arrangement as the
[600 series](../philips-600-series#teardown--disassembly). That exposes the
control PCB.

## Reverse-engineering log

The running plan, roughly in order. `tbc` — this list grows as things are
learned.

**1. Get a device.** ✅ Done.

**2. Work out how to open it.** ✅ Done — twist the top to the left, same as the
600 series. See [above](#opening-the-unit).

**3. Identify the MCU, pull its datasheet.** ⬜ Open.
The large unmarked QFP in the centre of the board is the main MCU. Knowing the
part gives its pinout, which narrows down *which* pins carry the UART instead of
guessing from the module side alone.
- Read the top marking (angled light / a phone macro shot helps).
- With the datasheet in hand, map the peripheral pins to the traces heading
  toward the MXCHIP module.

**4. Sniff traffic on those pins.** ⬜ Open.
- Buzz out which module pads are UART **TX/RX** vs. power/reset — the series
  resistors clustered at the pads (`R168`–`R171`, `R20`, `R21`, `R24`, `R81`)
  are the obvious probe points.
- Confirm the **baud rate** — 115200 8N1 on the 600 series, unverified here.
- Capture **both directions** during app and button interaction with
  [`philips-900-uart-sniffer.yaml`](./philips-900-uart-sniffer.yaml) or a logic
  analyzer; results go in [`captures/`](./captures).
- Visually trace what *other* pins are in use — there may be more than a plain
  two-wire UART between MCU and module (reset, boot-mode, an enable line).
- Check whether the frames use the same `FE FF` framing the
  [`philips`](../../components/philips) component already speaks, or something new.

**5. Is the air-quality sensor wired to the MCU or to the MXCHIP?** ⬜ Open.
This decides how much of the job the MCU protocol actually covers. The connector
silkscreened **PM2.5** sits at the bottom-right of the board, close to the
module — close enough that it is worth tracing rather than assuming it lands on
the MCU. If the sensor talks to the MXCHIP directly, replacing the module means
picking up the sensor as well (which the ESP32 can read natively — the 600
series' PM1003 is a plain UART part).

**Still to schedule:**
- Find how to **park the stock module** — the 600 series pulls the original
  ESP32's `EN` low; the equivalent pin on the EMC6069 needs identifying.
- Read the chip marking under the module can (or from the
  [FCC internal photos](https://fccid.io/P53-EMC6069)) to settle the
  reflash-vs-replace question for good.
- Add `AC0950` / `AC0951` to `VALID_MODELS` in
  [`components/philips/__init__.py`](../../components/philips/__init__.py)
  once the protocol is confirmed.

## What the teardown answered

The root README's [open question](../../README.md#-open-question--can-the-mxchip-models-run-esphome-after-all)
asked *"which MXCHIP part is in there"* — for the 900 series it is now answered:

**MXCHIP `EMC6069-P`**, marked `EMC6069-HF` on the daughterboard silkscreen,
FCC ID `P53-EMC6069`, certified 2023-12-20 as a 2.4 GHz Wi-Fi/BLE combo module.

What that means for the reflash idea:

- It is **not** an EMW3080 / MX1290, so the known-good
  RTL8710BN → [LibreTiny](https://docs.libretiny.eu/) → ESPHome path does
  **not** automatically apply here.
- MXCHIP does not publish the EMC6069's silicon, and the FCC filing keeps the
  block diagram and schematics confidential — only the user manual, test
  reports and internal photos are public. The die/chip marking would have to be
  read off the module itself (or from the filing's internal photos) before
  anyone can say whether it is a LibreTiny target.
- So for now the working assumption is the 600-series one: **add an ESP32 and
  park the stock module**, rather than reflash it.

## Board observations

From the control-PCB photos (see [`images/`](./images)):

- The MXCHIP module sits on the right-hand side of the board, castellated pads
  along its top and bottom edges, with series resistors (`R168`–`R171`, `R20`,
  `R21`, `R24`, `R81`) clustered right at those pads — the usual place to find
  the UART lines and a convenient place to probe them.
- A large unmarked QFP in the centre of the board is the main MCU.
- Piezo buzzer top-centre, several JST fan/motor connectors down the left edge.
- A small connector near the bottom-right is silkscreened **PM2.5** — presumably
  where the AC0951's particulate sensor plugs in (see step 5 above).

## Capturing the UART

1. Copy [`secrets-example.yaml`](./secrets-example.yaml) to `secrets.yaml` and
   fill in your Wi-Fi credentials.
2. Wire an ESP32-C3 to the board **read-only** — GND plus one GPIO per
   direction, no TX. Leave the stock MXCHIP module powered and running: the
   point is to record its conversation with the MCU.
3. Flash `philips-900-uart-sniffer.yaml` and watch the logs while you drive the
   purifier from its buttons and from the Philips Air app.
4. Save the log to [`captures/`](./captures) — the 600-series
   [`captures/`](../philips-600-series/captures) folder shows the format that
   worked there.

## Files

| File | Status | Purpose |
|------|--------|---------|
| [`philips-900-uart-sniffer.yaml`](./philips-900-uart-sniffer.yaml) | ✅ usable | Passive both-direction UART capture — the only flashable config here |
| [`common.yaml`](./common.yaml) | ⚠️ WIP | Shared entity config for the eventual component support |
| [`philips-ac0950.yaml`](./philips-ac0950.yaml) | ⚠️ WIP | AC0950 board config |
| [`philips-ac0951-c3_dev.yaml`](./philips-ac0951-c3_dev.yaml) | ⚠️ WIP | AC0951 board config, adds PM2.5 entities |
| [`secrets-example.yaml`](./secrets-example.yaml) | ✅ | Template — copy to `secrets.yaml` |
| [`captures/`](./captures) | — | UART captures go here |
| [`images/`](./images) | — | Teardown / PCB photos |
