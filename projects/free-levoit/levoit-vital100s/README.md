[← Back to Free Levoit Project](../README.md)

# Levoit Vital 100S - Custom Firmware (ESPHome)

Started from community projects and evolved into a generic Levoit ESPHome component for Core/Vital models.

See [Levoit Component](../../../components/levoit/README.md) for complete component documentation.

## Quick Facts

| Item | Value |
| --- | --- |
| Model | Vital 100S |
| Tested MCU FW | 1.0.5 |
| ESPHome | 2025.12.5+ |
| Speeds | 5 levels |
| CADR (spec) | ~400 m³/h |
| Entities | Fan (manual/auto/sleep), Current CADR, Filter Life Left, Filter Low (binary), Reset Filter Stats (button) |

## Features

* Fan component with modes (Manual, Auto, Sleep, Pet mode) and 5-speed control
* Current CADR sensor (m³/h), updated every few seconds; Filter Life Left (%) sensor
* Filter Low binary sensor (<5%)
* Reset Filter Stats button (resets CADR/runtime counters)
* Filter lifetime configurable (months), tracked from runtime and speed
* TLV-based protocol with extensible command/response structure
* Display control and brightness adjustment

## Disassembly

The Vital 100S uses a similar form factor to Core models:

* Place device upside down and remove the base cover
* Remove the air filter and any clips holding internal components
* Locate the 8 screws holding the chassis (4 typically have washers)
* Remove all screws carefully—they're made of soft metal
* Using a pry tool, carefully separate the base from the top sleeve
* Unplug the main logic board from its connectors

## Debug Header Pinout

The Vital series typically has a debug header or solder pads near the ESP32:

* Pin 1: TX (MCU TX → ESP RX)
* Pin 2: RX (MCU RX → ESP TX)
* Pin 3: GND
* Pin 4: 3.3V
* Pin 5: IO0 (for bootloader mode)
* Pin 6: EN (reset)

## Flash

* Solder wires to the debug header pins or use a pogo pin connector for easier access
* Connect to a USB-UART adapter (TTL 3.3V), making sure TX/RX are crossed:
  - Adapter TX → MCU RX
  - Adapter RX → MCU TX
* Connect IO0 to GND during power-up to enter bootloader mode
* Disconnect IO0 once in bootloader

### Backup Existing Firmware

```bash
esptool read_flash 0 ALL levoit_vital100s.bin
```

Note: Some devices may have watchdog protection. Try backing up while powered externally.

### Update Configuration and Secrets

Rename `secrets-example.yaml` to `secrets.yaml` and set your WiFi credentials and Home Assistant encryption key.

Adopt the device name in `levoit-vital100s.yaml` if you have multiple units.

See [Levoit Component](../../../components/levoit/README.md) for complete component documentation.

### Compile and Install New Firmware

```bash
esphome run levoit-vital100s.yaml
```

Once flashing completes, reassemble the device and enjoy!

#### Restore Original Firmware (if needed)

```bash
esptool erase_flash
esptool write_flash 0x00 levoit_vital100s.bin
```

## Protocol Notes

The Vital 100S uses the **TLV (Type-Length-Value)** protocol for communication:

* All responses use TLV blocks (ID, length, value pairs)
* Most commands also use TLV encoding for extensibility
* This differs from the fixed-field protocol used by Core models
* See [main project README](../README.md) for protocol details and TLV ID mappings
