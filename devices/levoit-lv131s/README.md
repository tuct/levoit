[← Back](../../README.md)

# Levoit LV-PUR 131S - Custom Firmware (ESPHome)

This model is discontinued and known for reliability issues (sudden shutdown syndrome). This project upgrades the original hardware with modern components while maintaining form factor compatibility.

The original PM1003 sensor was unreliable; spare PM5003 components were substituted for better performance. Legacy PM1003 code is preserved as commented reference.

A hardware hack: the original ESP12F is removed (or its power cut) and a XIAO
ESP32-C3 is wired to the board in its place. The steps shared by all devices are in the
[install guide](../../docs/installing.md); this page has what is particular to
the LV-PUR 131S.

## Quick Facts

| Item | Value |
|------|-------|
| Model | LV-PUR 131S (discontinued) |
| MCU | Original ESP12F → [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link) |
| Sensor | Original PM1003 → PM5003 (significantly improved air quality detection) |

### Required Parts

- Broken or working Levoit LV-PUR131S
- [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link)
- PM5003 particle sensor
- Wires and some connectors

### Required Tools

- Soldering Iron

## Features

* **Home Assistant Integration** (no cloud required)
  * Full fan support with 3 manual speeds + Auto/Sleep modes
  * Filter lifetime tracking based on actual runtime and usage
  * Display on/off control and brightness
  * Reset filter button (hold 5s until beep, or trigger via HA)
  * Timer support (30 minutes to 12 hours)

* **Unique Features**
  * Hidden Doom! easter egg (hold Sleep button 4 seconds)
  * Configurable sound feedback (buttons only, not remote control)
  * Improved PM2.5 sensor (PM5003 vs original PM1003)

![Home Assistant Dashboard](./images/home_assistant.png)
![Home Assistant Dashboard (continued)](./images/home_assistant_2.png)

[Link to Video](https://www.youtube.com/watch?v=NGHwQ--Szvg)

[![Link to Video](./images/video.png)](https://www.youtube.com/watch?v=NGHwQ--Szvg)

## Reliability Issues - "Sudden Death" Syndrome

This model experienced manufacturing defects leading to premature failures. Resources:

* [Reddit Discussion - Manufacturing Defect](https://www.reddit.com/r/AirPurifiers/comments/1idc5nu/levoit_air_purifiers_manufacturing_defect/)
* [YouTube - Diagnosis and Repair](https://www.youtube.com/watch?v=-IfPcGs717E)
* [YouTube - Component-Level Analysis](https://www.youtube.com/watch?v=RJjbAqp-lw4)

**Root Cause:** Overvoltage transients reaching the MCU/PCB, typically burning the voltage regulation diode and main microcontroller.

**Experience:** The original ESP12F in this device was destroyed; diode damage was also observed. A replacement unit obtained under warranty enabled reverse engineering and development of this custom ESPHome firmware.

## 1. Build the firmware

**With [Home Assistant](../../docs/installing.md#with-home-assistant-recommended):** No Home Assistant template yet: build it manually.

**Manually:** the files in this folder, built with the ESPHome CLI or pasted
into the Device Builder. [Step by step](../../docs/installing.md#manually).
Requires ESPHome 2026.01+

| Board | ESPHome CLI / Device Builder |
|-------|------------------------------|
| Seeed XIAO ESP32-C3 | `levoit-lv131s-c3.yaml` |
| ESP32 dev board (`az-delivery-devkit-v4`) | `levoit-lv131s.yaml`; its first line says it needs a custom ESPHome version (500Khz for SPI) |

## 2. Disassembly

[Video - Start to 1:40](https://www.youtube.com/watch?v=RJjbAqp-lw4)

- Remove all screws on the outside (white plastic), 3 on bottom, 2 top (left and right, long screwdriver!).
- Remove the Filter
- Remove all screws, one is a triangle shaped one!
- Open the front carfully from the bottom

My power supplies are still working, if needed get a 24V DV like a Meanwell LRS 50-24 + aa 24V to 5V bucket converter.
![new sensor](./images/power_board.jpg)

- Unplug the Switch for checking if the back is open
- Unplug the connection to the MCU
- Unscrew all screws holding the Fan base in
- Remove the Fan base from the bottom
- Unplug the Sensor and unscrew it
- To remove the PCB (Optional!?)
  - First screw can easily be unscrewed
  - i used pliers for the other 2 ones, as i would need to remove the top panel and i did not find a way todo this undestrutive.

## 3. Modify the PCB

Might be possible without removing the PCB! i did not try but it seems there is enough room!
Soldering is a bit hard/wired, some protective film seems to be applied.
![pcb siwith wiresde](./images/board_wired.jpg)

- Remove the old MCU or cut power and gnd at least
- Unplug the old sensor from the case
- I used Power and GND from the old sensor and added 2 wires for the UART of the PM 5003, this allows to still use the reset BTN!
  ![new sensor](./images/new_sensor_1.jpg)
  ![new sensor](./images/new_sensor_2.jpg)
  ![new sensor](./images/new_sensor_3.jpg)
  ![new sensor](./images/new_sensor_4.jpg)
  ![pm5003 pinout](./images/PMS5003-PINOUT.jpg)
- Solder the wires for the new MCU
  ![PCB with wires attached](./images/board_wired2.jpg)
  Note: D5 resistor is not required if PM5003 sensor is used.

* Flash Firmware: the config from [step 1](#1-build-the-firmware)

Which GPIO of the original ESP12F drives the display, buttons, sensor, fan and
beeper: [hardware notes](./PROTOCOL.md#hardware).

## 5. Assembly

* Connect the new MCU to the PCB and secure with hot glue:
  ![MCU mounted on PCB](./images/pcb_hacked_mounted.jpg)
  ![MCU detail view](./images/pcb_hacked_mounted_2.jpg)

* Position cables correctly: bottom cable on right side, sensor cables at top
  ![Cable routing](./images/assembly_0.jpg)

* Carefully insert the fan unit—ensure it sits fully at the top and no cables are pinched
  ![Fan assembly step 1](./images/assembly_1.jpg)

* Connect the MCU back to power board and verify power connections
  ![Final power connections](./images/assembly_2.jpg)

* Close the rear lid and screw in all fasteners
  ![Lid reassembly](./images/assembly_3.jpg)

**Done!** Your upgraded LV-PUR 131S is ready for use.
[What to check](../../docs/installing.md#5-assembly) once it is powered.

## Hardware & protocol notes

The original board (ESP12F pin use, TM1628 display driver, button serial line,
PM1003, fan PWM, beeper) and the display byte map are in
[PROTOCOL.md](./PROTOCOL.md).
