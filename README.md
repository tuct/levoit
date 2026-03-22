

# Project - Free Levoit Air Purifiers

Collection of custom ESPHome firmware and hardware projects for Levoit air purifiers, eliminating cloud dependency and enabling native Home Assistant integration.

Here is a list of all 5 models running with ESPHome. Core 300S and Core 400S only require disassembly to flash firmware (no hardware modifications needed). The Mini and LV-PUR 131S require custom hardware hacks and PCB modifications.

* [Generic Levoit ESPHome Component](./components/levoit/), full custom ESPHome-based firmware

* [levoit-core300s](./devices/levoit-core300s) – Custom ESPHome Firmware
* [levoit-core400s](./devices/levoit-core400s) – Custom ESPHome Firmware
* [levoit-vital100s](./devices/levoit-vital100s) – Custom ESPHome Firmware
* [levoit-lv-pur131s](./devices/levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [levoit-mini](./devices/levoit-mini) – Custom PCB, 3D parts, hardware hack

Soon:
* [Core 200s](./devices/levoit-core200s) – Custom ESPHome Firmware - WIP
* [Core 600s](./devices/levoit-core600s) – Custom ESPHome Firmware - WIP
* [Vital 200S](./devices/levoit-vital200s) – Custom ESPHome Firmware - WIP

## Install Methods (ESP32)

Choose the approach that fits your device and risk tolerance:

1) **Reuse Original ESP32 (single ESP)**
  - Put original ESP32 in bootloader, flash ESPHome directly over UART (TX/RX/GND/EN/GPIO0).
  - Simplest wiring; you lose the stock firmware unless you back it up first.

2) **Add Second ESP32 (dual ESP)**
  - Keep the factory ESP32 intact and add a new ESP32 in parallel on UART.
  - Use a 2-position switch on EN to select which ESP32 is active (switch only while powered down).
  - Lets you revert to stock firmware or take MCU updates; recommended for cautious installs.

3) **Replace Module**
  - Desolder/disable the factory ESP32 module and drop in your own (e.g., ESP32-S3/C3).
  - Cleanest for long-term custom firmware; cannot easily revert to stock without rework.

General tips:
- Backup with `esptool read_flash 0 ALL levoit.bin` if possible.
- Common wiring: 3V3, GND, TX→MCU RX, RX→MCU TX, EN, GPIO0 (for boot).
- For WiFi LED/filter LED behaviors and entity list, see the component README.

Not my projects, but worth checking out:
* [levoit-vital-200s + levoit-vital-200s pro, levoit-vital-100s?](https://github.com/targor/levoit_vital/?tab=readme-ov-file)
* https://github.com/mulcmu/esphome-levoit-core300s
* https://github.com/acvigue/esphome-levoit-air-purifier


Helpfull links:
* [How to open Levoit's](https://www.youtube.com/watch?v=6wxHpUVcGFc) 
* [How to open Levoit's smaller](https://www.youtube.com/watch?v=rAjLNR1jQkw)
