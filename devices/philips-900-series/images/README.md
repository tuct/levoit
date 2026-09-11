Teardown, install and PCB photos for the Philips Series 900 (AC0950 / AC0951).

All photos are of an **AC0951**. EXIF has been stripped, and the MXCHIP module's
MAC is blurred wherever the label is legible.

## Board

| File | What it shows |
|------|---------------|
| `ac0951_pcb.jpg` | The whole control PCB in situ, component side — MXCHIP module at the right, the unmarked main MCU (QFP) in the centre, buzzer top-centre, fan/motor JST connectors down the left edge, and the `PM2.5` connector bottom-right. |
| `mxchip_emc6069.jpg` | Close-up of the MXCHIP module. The label reads `EMC6069-P`, `FCC ID: P53-EMC6069`; the daughterboard silkscreen reads `EMC6069-HF`. The series resistors at its pads (`R168`, `R20`, `R21`, `R81`) are visible around it. |
| `ac0951_pcb_detail.jpg` | Close-up of the power section and the lower module pads — `R168`, `R20`, `R21`, `R31`, `R181` and the `J2` header, with the module's bottom row of castellations at the top right. |
| `ac0951_pcb_display_side.jpg` | The user-facing side of the same board: three button springs, the LED matrix that DP `0x04` dims, the status LEDs, and the board markings `TL-2780-C-V1.0` / `TLC2700-C-V1.0`. |
| `mxchip_emc6069_pins.jpg` | **The wiring guide photo.** Same view, annotated: `+5V` and `GND` on the 4-pin header, **A** (MCU → module, goes to ESP32 RX), **B** (module → MCU, goes to ESP32 TX) and **C** (module reset/enable, pulled low to park it). Used by [Adding your own ESP32](../README.md#adding-your-own-esp32). |

## Install

The working AC0951 conversion, in order — see
[The install](../README.md#the-install).

| File | What it shows |
|------|---------------|
| `xiao_esp32c3.jpg` | The Seeed XIAO ESP32-C3 with its external antenna, before fitting. |
| `install_01.jpg` | ESP32-C3 wired to the control board — power, GND and the two UART lines. |
| `install_02.jpg` | Same, wider, showing where the wires run. |
| `install_03.jpg` | ESP32-C3 positioned in the cavity above the fan. |
| `install_04.jpg` | Board and ESP seated back in the housing. |
| `install_05.jpg` | Wire routing before closing up. |
| `install_06.jpg` | Ready to refit the top cap. |
| `top_cap_orientation.jpg` | **Reassembly key.** The slats on the inner ring are evenly spaced except one, which is wider — that is what tells you how to line the cap up before twisting it home. |
| `installed_running.jpg` | The reassembled unit powered up and running ESPHome. |

## Capture rig

| File | What it shows |
|------|---------------|
| `uart_capture_setup.jpg` | The logic analyzer wired to the MCU↔module UART with the board hinged out — how every capture in [`../captures`](../captures) was taken. |
| `uart_capture_running.jpg` | The same rig mid-capture with the unit reassembled. |

## Still wanted

- **`mcu_marking.jpg`** — the centre QFP's top marking, for step 3 of the
  reverse-engineering log. It is not legible in any photo here; the part looks
  unmarked under flat light, so it needs angled light or a macro shot.
- **`pm25_connector.jpg`** — where the `PM2.5` connector's traces actually go.
  The captures already prove the MCU owns the readings, so this is now just
  confirmation.
- **`overview.jpg`** — the assembled unit, to match the 600-series folder.
- **Anything from an AC0950.** Every photo here is an AC0951.
