# Levoit ESPHome Component

Custom ESPHome component for Levoit air purifiers (Core and Vital series) enabling local control without cloud dependency.

## Supported Models

| Model | MCU Version | Status |
|-------|-------------|--------|
| Levoit Vital 100S | 1.0.5 | ✅ Tested |
| Levoit Vital 200S | - | ⚠️ Untested |
| Levoit Core 300S | 2.0.11 | ✅ Tested -> with new ESP!|
| Levoit Core 400S | 3.0.0 | ⚠️ Untested |

**Requirements:** ESPHome 2025.12.5+

## Features

### Fan Control
- Standard fan entity with multiple speed levels (1-4)
- Preset modes for Auto, Sleep, and Manual operation
- Pet mode (Vital series)
- On/Off control

### Sensors
- **PM2.5**: Real-time particulate matter readings (Core series)
- **Timer Status**: Remaining time display (e.g., "5h 33 min remaining")
- **Filter Life**: Monitoring and reset capability (planned)

### Configuration Options
- **Timer**: Set duration in minutes
- **Auto Mode**: Configurable with room size optimization
  - Default mode
  - Quiet mode
  - Efficient mode (with room size parameter)
- **Display Control**: Toggle screen on/off
- **Light Detection**: Automatic display brightness adjustment (Vital series)
- **Child Lock**: Prevent physical button presses
- **Display Lock**: Lock display settings

## Installation

### Hardware Setup

- ⚠️ Requires disassembly and serial access (TX, RX, GND, EN, GPIO0) initially


#### Option 1: Flash Original ESP32 Directly
Flash ESPHome directly onto the factory ESP32-Solo-1 module using serial connection.

#### Option 2: Dual ESP Setup (Preserve Original)
Keep the original ESP32 functional while adding a custom ESP32 for ESPHome control. This approach allows switching between firmware versions and enables MCU firmware updates.

**Hardware Setup:**
1. Install a **2-position switch** to select which ESP32 is active, only use during power down!
2. Wire the switch:
   - **Common (middle)**: Connect to GND
   - **Position A**: Connect to EN pin of original ESP32
   - **Position B**: Connect to EN pin of new ESP32

3. Connect new ESP32:
   - Power (3.3V) and GND from purifier PCB
   - TX/RX to MCU UART pins (parallel to original ESP32)

**Benefits:**
- ✅ Revert to factory firmware anytime
- ✅ Perform official MCU firmware updates when needed
- ✅ Test ESPHome changes without risk
- ⚠️ **Note**: New MCU firmware may require protocol updates in this component


**Recommended modules:**
- **XIAO Seeed ESP32-S3** - Compact form factor, easy to integrate
- **XIAO Seeed ESP32-C3** - Budget-friendly alternative
- Any ESP32 module with UART and sufficient GPIO pins












### Hardware Access

Each model requires different disassembly procedures. See model-specific guides in [projects/free-levoit](../../projects/free-levoit/) for:
- PCB pinout diagrams
- Disassembly instructions
- UART pin locations
- Photos and wiring diagrams



### Software Installation

#### Step 1: Add External Component
In your ESPHome YAML configuration:

**Local Development:**
```yaml
external_components:
  - source:
      type: local
      path: ../../../components  # Relative to your YAML file
    components: [levoit]
```

**Production (GitHub):**
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/tuct/esphome_external_components
      ref: main
    components: [levoit]
```

#### Step 2: Configure UART
Match your hardware connections:
```yaml
uart:
  tx_pin: GPIO4   # ESP TX → MCU RX
  rx_pin: GPIO5   # ESP RX → MCU TX
  baud_rate: 115200
```

#### Step 3: Add Levoit Component
Specify your model (must match device):
```yaml
levoit:
  id: air_purifier
  model: CORE300S  # Options: VITAL100S, VITAL200S, CORE300S, CORE400S
```

#### Step 4: Compile and Flash
```bash
# Compile only (check for errors)
esphome compile your-config.yaml

# Compile and upload via serial
esphome run your-config.yaml

# Upload via OTA (after initial flash)
esphome upload your-config.yaml --device your-device.local
```

For complete working examples, see the [free-levoit project configurations](../../projects/free-levoit/).

### Troubleshooting Installation

**No communication with MCU:**
- Verify TX/RX are not swapped (common mistake)
- Check 3.3V power supply voltage under load
- Enable UART debugging: `uart: debug: { direction: BOTH }`
- Confirm baud rate is 115200

**Boot loops or crashes:**
- Check `model:` matches your actual device
- Verify GPIO pins don't conflict with ESP32 bootstrap pins
- Try disabling PSRAM if using ESP32-S3: `psram_mode: disabled`

**Device not responding in Home Assistant:**
- Confirm ESPHome API is enabled
- Check WiFi credentials and network connectivity
- Review logs: `esphome logs your-config.yaml`
- Verify model detection: Look for "Model set to: CORE300S (ModelType=2)" in logs




## Architecture

### Component Structure
```
levoit/
├── levoit.cpp/.h           # Main component, UART handling, message routing
├── decoder.cpp/.h          # Frame parsing and message dispatch
├── types.h                 # Enum definitions for commands and entity types
├── core_status.cpp/.h      # Core series status/timer payload decoders
├── vital_status.cpp/.h     # Vital series status payload decoders
├── core_commands.cpp       # Core series command builders (300S/400S)
├── vital_commands.cpp      # Vital series command builders (100S/200S)
├── decoder_helpers.h       # Shared utility functions
├── tlv.cpp/.h             # TLV encoding for complex payloads
└── [platform]/            # ESPHome platform integrations
    ├── fan/               # Fan entity
    ├── switch/            # Switch entities (display, lock, etc.)
    ├── number/            # Number entities (timer, room size)
    ├── select/            # Select entities (auto mode)
    ├── sensor/            # Sensor entities (PM2.5, etc.)
    └── text_sensor/       # Text sensor entities (timer display)
```

### Protocol Details
- **Interface**: UART at 115200 baud
- **Frame Format**: `A5 [type:3] [seq:1] [len:1] [reserved:1] [chk:1] [cmd:1] [payload:N]`
- **Byte Order**: Little-endian for multi-byte values
- **Model Detection**: Automatic based on initial handshake (TODO)
- **Checksum**: Sum of all bytes excluding checksum byte itself


## Configuration Example

```yaml
external_components:
  - source:
      type: local
      path: ../../../components  # or use git source
    components: [levoit]

uart:
  tx_pin: GPIO4
  rx_pin: GPIO5
  baud_rate: 115200

levoit:
  id: air_purifier
  model: CORE300S  # or VITAL100S, VITAL200S, CORE400S
  
fan:
  - platform: levoit
    levoit_id: air_purifier
    name: "Air Purifier"

switch:
  - platform: levoit
    levoit_id: air_purifier
    display:
      name: "Display"
    child_lock:
      name: "Child Lock"

number:
  - platform: levoit
    levoit_id: air_purifier
    timer:
      name: "Timer"
      min_value: 0
      max_value: 720  # 12 hours
      step: 1
```

For complete configuration examples, see the [free-levoit project](../../projects/free-levoit/).

## Development Notes

### Building
PlatformIO requires force-including command implementation files in [levoit.cpp](levoit.cpp):
```cpp
#include "core_commands.cpp"
#include "vital_commands.cpp"
```

### Adding New Commands
1. Add enum to `CommandType` in [types.h](types.h)
2. Implement in `build_core_command()` or `build_vital_command()`
3. Pattern: Define `msg_type` and `payload` vectors, return `build_levoit_message(msg_type, payload)`

### Debugging
Enable verbose logging in your YAML:
```yaml
logger:
  level: VERBOSE
  
uart:
  debug:
    direction: BOTH  # Monitor raw UART traffic
```

## Known Issues & TODO

- [ ] Wifi Led blink and light after connect
- [ ] Add filter life time and current CADR sensors
- [ ] Enable filter reset from Home Assistant
- [ ] Implement custom sleep mode settings for Vital series
- [ ] Model-specific room size validation based on CADR ratings
- [ ] Verify Core 300S/400S protocol differences (MCU version dependency)
- [x] Add appropriate icons for entities
- [x] Test compatibility with ESPHome 2025.11+ (preset mode API changes)


## Credits

Special thanks to the original developers who reverse-engineered the Levoit protocols:
- [targor](https://github.com/targor/) - [Levoit Vital integration](https://github.com/targor/levoit_vital/)
- [mulcmu](https://github.com/mulcmu/) - [Levoit Core 300S](https://github.com/mulcmu/esphome-levoit-core300s)
- [acvigue](https://github.com/acvigue/) - [Levoit Core integration](https://github.com/acvigue/esphome-levoit-air-purifier)

## License

This component is provided as-is for educational and personal use. Levoit and related trademarks are property of their respective owners.

