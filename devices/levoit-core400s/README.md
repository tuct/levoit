[← Back to Free Levoit Project](../README.md)

# Levoit Core 400S - Custom Firmware (ESPHome)

Started from community projects ([acvigue](https://github.com/acvigue/esphome-levoit-air-purifier), [mulcmu](https://github.com/mulcmu/esphome-levoit-core300s)) and evolved into a generic Levoit ESPHome component for Core/Vital models.

See [Levoit Component](../../../components/levoit/README.md) for complete component documentation.

## Quick Facts

| Item | Value |
| --- | --- |
| Model | Core 400S |
| Tested MCU FW | 3.0.0 (status: untested) |
| ESP | ESP32-SOLO-1C |
| Board | CORE400S Ctrl V1.2 |
| Speeds | 4 levels |
| CADR (spec) | 415 m³/h |
| ESPHome | 2025.12.5+ |
| Entities | Fan (manual/auto/sleep), Current CADR, Filter Life Left, Filter Low (binary), Reset Filter Stats (button) |

## Features

* Fan component with modes (Manual, Auto, Sleep)
* Current CADR sensor (m³/h), Filter Life Left (%) sensor
* Filter Low binary sensor (<5%)
* Reset Filter Stats button (resets CADR/runtime counters)
* Filter lifetime configurable (months), tracked from runtime and speed
* Display run time in Home Assistant

## Disassembly

Very similar to Core 300s but i had to unplug the pcb from the carry holes to get to it after i removed the filter and filter hosing.
* Place upside down and remove base cover and filter to expose 8 screws (4 have washers)
* Remove all 8 screws be careful, as these are made out of a soft metal
* Using a pry tool slide in between tabs
* Separate base and top sleeve
* Unplug logic board - done with screwdriver/kitchen knife from the side
* Remove Fan unit to get to the logic board

J1 - dubug header pinout
1 - TX
2 - RX
3 - GND
4 - 33.3V
5 - IO0
6 - EN

I was not able to dump the original FW (may be watchdog-protected). Try while the PCB is powered.

## Flash

* Solder wires to pins TXD0, RXD0, IO0, +3V3, and GND near the ESP32 on the logic board, and connect these to a USB-UART converter. On some boards, if these are through holes, soldering may not be necessary.
* Connect IO0 to ground during power before connecting USB-UART to boot to bootloader. On some boards, IO0 may not have it's own debug pin and the ESP32 GPIO0 pin on the esp can be used.

### Backup Existing Firmware
```
esptool read_flash 0 ALL levoit.bin
```

This did not work for me, always ended in an error, so i yoloed it and continued without a backup of the original FW

Rename `secrets-example.yaml` to `secrets.yaml` and set your WiFi and encryption key.

Adopt device name in `levoit-core400s.yaml` if you have multiple units.

See [Levoit Component](../../../components/levoit/README.md) for complete component documentation.

### Compile and Install New Firmware

```bash
esphome run levoit-core400s.yaml
```

Reassemble and enjoy!

#### Restore Original Firmware (if needed)

```bash
esptool erase_flash
esptool write_flash 0x00 levoit.bin
```







