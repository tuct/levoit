[← Back](../../README.md)

# Levoit Sprout - Custom Firmware (ESPHome)

Air purifier with 4 fan speeds. Unlike other Levoit models, the Sprout has two unique hardware features:

- **White noise** — plays one of 15 built-in sounds stored as MP3s on the ESP32 flash (LittleFS `assets` partition). Volume and sound selection are controlled by the ESP32, not the MCU.
- **LED ring** — decorative RGB light ring on the front, controllable in nightlight (static color + brightness) or breathing (pulsing) mode.

Uses the Vital flat-TLV protocol (`CMD=02 00 55`) as a base, with Sprout-specific extensions for the LED ring, white noise, PM sensors, and MCU async events.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Sprout |
| Tested MCU FW | 1.0.5 |
| ESP Module | ESP32-D0WDR2-V3 |
| Flash | 8 MB |
| Fan Speeds | 4 |
| CADR (spec) | 144.5 m³/h |
| Room Size | 9-30 m² |
| ESPHome | 2026.1.2+ |

## Features

### Shared with Vital

| Feature | Type | Notes |
|---------|------|-------|
| Fan | fan | 4 speeds, presets: Manual / Auto |
| Auto Mode | select | Default / Quiet / Efficient |
| Auto Mode Room Size | number | m² |
| Auto Mode High Fan Time | text_sensor | Remaining high-speed runtime in efficient mode |
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

### Sprout-specific

#### Air Quality Sensors (direct to ESP32)

| Sensor | Chip | Interface | Measures |
|--------|------|-----------|----------|
| Temperature + Humidity | Sensirion SHT4x | I2C | °C / %RH. Also used to compensate SGP40 readings |
| VOC Index | Sensirion SGP40 | I2C | 1–500 VOC index |
| CO2 | Winsen MH-Z1911A | UART or PWM | True NDIR CO2, 400–5000 ppm. Mounted on carrier board "Baymax-CO2-Senser-V1.1SP2" |


MCU - Fan / etc
RX: GPIO23
TX: GPIO22

I2C - Sensirion SGP40 / Sensirion SHT4x
GPIO14 - SCL
GPIO27 - SDA

Uart MH-Z1911A
Baud: 9600
RX: GPIO02
TX: GPIO15
or reverse?

**I2S Audio** (confirmed by logic analyzer capture)

| GPIO | I2S Signal | Description |
|------|-----------|-------------|
| GPIO 5 | BCLK | Bit clock ~1 MHz (= 16 bits × 2 ch × 32kHz) |
| GPIO 18 | LRCLK / WS | Word select 32 kHz — toggles L/R channel |
| GPIO 17 | SD | Serial data |
| GPIO 19 | ENABLE / MUTE | Amp or codec enable on/off |



#### Via MCU (UART protocol)

| Feature | Type | Notes |
|---------|------|-------|
| PM 1.0 | sensor | Raw count from PM sensor (tag 0x0C) |
| PM 10 | sensor | Raw count from PM sensor (tag 0x0D) |
| Fan RPM | sensor | Fan tachometer (tag 0x27) |

#### ESP32-controlled

| Feature | Type | Notes |
|---------|------|-------|
| LED Ring | switch | Decorative front LED ring on/off (restores nightlight mode) |
| Light Mode | select | Off / Nightlight / Breathing |
| LED Brightness | number | Nightlight brightness AND breathing max brightness (0–4095 LE16) |
| LED Color Temperature | number | Nightlight color temperature (0–255, default 58) |
| LED Brightness Min | number | Breathing mode minimum brightness (0–255) |
| LED Breathing Time | number | Breathing cycle duration in seconds (1–10) |
| White Noise | switch | Enable / disable white noise playback |
| White Noise Sound | select | Sound 01–15 (MP3/OGG stored in ESP32 flash — see note below about downloading all sounds) |
| White Noise Volume | number | Playback volume (0–255). Handled by ESP32, not MCU |
| Last MCU Event | text_sensor | Last async event from MCU (button press, white noise state change, cover) |
| Cover Open | binary_sensor | Filter cover/door open sensor |

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

## Teardown / Disassembly

> TODO: add teardown steps and photos

Twist open the top head counter clockwize

## Debug Header Pinout

> TODO: add pinout and photos

## Wiring New ESP

> TODO: add wiring photos and pin mapping

## Flash

### Prerequisites

Connect to the debug header with a USB-UART adapter (3.3V TTL), crossing TX/RX:
- Adapter TX → MCU RX
- Adapter RX → MCU TX

Connect **IO0 to GND before powering on** to enter bootloader mode.

### Backup Existing Firmware

```bash
esptool.py --no-stub --chip esp32 -b 460800 read_flash 0 0x800000 firmware-backup.bin
```

Extract audio assets (MP3s from LittleFS `assets` partition):
```bash
pip install littlefs-python
python extract_assets.py firmware-backup.bin
# output written to assets_extracted/
```

> **Important:** Only 5 of the 15 white noise sounds are pre-loaded in flash. The rest must be downloaded via the original app before flashing. **Before replacing the firmware:**
> 1. Open the original Levoit app → White Noise
> 2. Add sounds **1–5** to the playlist and tap **Save** — the ESP32 will start downloading them
> 3. Wait for the upload to finish, then add sounds **6–10** → **Save**
> 4. Wait, then add sounds **11–15** → **Save**
>
> Then re-dump the flash (`read_flash 0 0x800000`) — all 15 sounds will now be stored in the `assets` partition.

### Audio Files

| File | Sound | Source |
|------|-------|--------|
| `100001.ogg` | Summer rain | Pre-loaded |
| `100002.mp3` | Gentle sea wave | Pre-loaded |
| `100006.mp3` | Insects chirp by the stream | Pre-loaded |
| `100009.mp3` | Morning seaside | Pre-loaded |
| `1000012.mp3` | Sound 12 (name TBD) | Pre-loaded |
| others | Remaining 10 sounds (names TBD) | Downloaded from app |
| `bgm.mp3` | UI background music | Pre-loaded |
| `switch.mp3` | Button click sound effect | Pre-loaded |

### Configure

1. Copy `secrets-example.yaml` → `secrets.yaml` and fill in your Wi-Fi and encryption key
2. Check and update the UART pin assignments in `levoit-sprout-c3.yaml`
3. Check the [component README](../../components/levoit/README.md) for UART pin mapping per board

### Flash

```bash
esphome run levoit-sprout-esp32.yaml
```

### ESPHome Web Builder / Dashboard

Use the pre-generated builder yaml to flash without a local clone — all config is inlined, no `!include` or packages needed:

| File | Board |
|------|-------|
| `levoit-sprout-builder.yaml` | original ESP32 |

Upload to the [ESPHome web builder](https://builder.esphome.io) or paste into the ESPHome dashboard. Regenerate with `.\make-builder-yaml.ps1` from the `devices/` folder.

### Restore Original Firmware

```bash
esptool erase_flash
esptool write_flash 0x00 firmware-backup.bin
```
