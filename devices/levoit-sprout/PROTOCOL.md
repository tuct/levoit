[← Back](./README.md)

# Sprout — hardware and protocol notes

## Protocol Notes

The Sprout uses the same Vital flat-TLV serial protocol. Additional Sprout commands:

| CMD | Direction | Function |
|-----|-----------|----------|
| `02 08 55` | MCU→ESP | Async events: tag01=button press/release, tag03=white noise state change `{active, vol, sound}`, tag04=cover open/close |
| `02 0B 55` | ESP→MCU | Nightlight: tag01=on/off, tag02=color temp (0–255), tag03=LE16 brightness (0–4095) |
| `02 0C 55` | ESP→MCU | Breathing: tag01=mode, tag02=cycle time (1–10s), tag03=LE16 max brightness, tag04=min brightness |
| `02 07 55` | ESP→MCU | White noise: tag01=`{active, volume, sound_index}` + constant trailer `03 03 05 08 F2` |
| `02 02 55` | ESP→MCU | White noise fan mode: payload `10 01 01` (enable) / `10 01 00` (disable) |

> **Note:** White noise volume and sound selection are handled entirely by the ESP32. The MCU only receives the final active/volume/sound state.
