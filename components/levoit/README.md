# Levoit ESPHome Component

Custom ESPHome component for Levoit air purifiers (Core and Vital series) enabling local control without cloud dependency.

## Supported Models

| Model | MCU Version | Status |
|-------|-------------|--------|
| Levoit Vital 100S | 1.0.5 | ✅ Tested |
| Levoit Vital 200S | - | ⚠️ Untested |
| Levoit Core 300S | 2.0.11 | ✅ Tested |
| Levoit Core 400S | 3.0.0 | ⚠️ Untested |

**Requirements:** ESPHome 2025.10.5+ (compatibility with 2025.11+ needs verification due to preset mode changes)

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

- [ ] Implement custom sleep mode settings for Vital series
- [ ] Add filter life time and current CADR sensors
- [ ] Enable filter reset from Home Assistant
- [ ] Model-specific room size validation based on CADR ratings
- [ ] Investigate: Room size resets after timer completion
- [ ] Verify Core 300S/400S protocol differences (MCU version dependency)
- [ ] Add appropriate icons for entities
- [ ] Test compatibility with ESPHome 2025.11+ (preset mode API changes)

## Credits

Special thanks to the original developers who reverse-engineered the Levoit protocols:
- [targor](https://github.com/targor/) - [Levoit Vital integration](https://github.com/targor/levoit_vital/)
- [mulcmu](https://github.com/mulcmu/) - [Levoit Core 300S](https://github.com/mulcmu/esphome-levoit-core300s)
- [acvigue](https://github.com/acvigue/) - [Levoit Core integration](https://github.com/acvigue/esphome-levoit-air-purifier)

## License

This component is provided as-is for educational and personal use. Levoit and related trademarks are property of their respective owners.

