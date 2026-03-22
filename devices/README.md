

# Project - Free Levoit Air Purifiers - Supported Device List

* [Levoit Core 200s](./levoit-core200s) – Custom ESPHome Firmware 
* [Levoit Core 300s](./levoit-core300s) – Custom ESPHome Firmware
* [Levoit Core 400s](./levoit-core400s) – Custom ESPHome Firmware
* [Levoit Vital 100s](./levoit-vital100s) – Custom ESPHome Firmware

Soon:
* [Levoit Core 600s](./levoit-core600s) – Custom ESPHome Firmware - WIP
* [Levoit Vital 200S](./levoit-vital200s) – Custom ESPHome Firmware - WIP

Special:
* [Levoit LV PUR 131s](./levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [Levoit Mini](./levoit-mini) – Custom PCB, 3D parts, hardware hack

Core 200S/300S/400S and Vital100/200, only require disassembly to flash firmware (no hardware modifications needed). 
They are using the esphome external component and can be flashed to original ESP32 or easily modded with a new one!
The Mini and LV-PUR 131S require custom hardware hacks and PCB modifications.

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





