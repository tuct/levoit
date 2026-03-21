[← Back to Components](../README.md)

# Levoit ESPHome Component

Custom ESPHome component for Levoit air purifiers (Core and Vital series) enabling local control without cloud dependency.

## Supported Models

| Model | MCU Version | Status |
|-------|-------------|--------|
| Levoit Vital 100S | 1.0.5 | ✅ Tested |
| Levoit Core 300S | 2.0.7, 2.0.11 | ✅ Tested -> with new ESP!|
| Levoit Core 400S | 3.0.0 | ✅ Tested -> with original ESP|
| Levoit Vital 200S | ?.?.? | ⚠️ Untested -> should be same as Levoit Vital 100S/200S pro |
| Levoit Vital 200S PRO | 2.0.0 | ✅ Tested -> with original ESP, Thanks @dnsefe |
| Levoit Core 200S | ?.?.? | ⚠️ Untested -> comming soon!|

**Requires:** ESPHome 2026.01.2+

## Change Log

### 2026.03.21

* Works with for esphome 2026.3+
* Sensors - added state class measurement -> allow statistics to be tracked

## Features

### Fan Control
- Standard fan entity with multiple speed levels (1-4, 3 for Core300s)
- Preset modes for Auto, Sleep, and Manual operation
- Pet mode (Vital series)
- On/Off control

### Sensors
- **PM2.5**: Real-time particulate matter readings (Core series)
- **Timer Status**: Remaining time display (e.g., "5h 33 min remaining")
- **Current CADR**: Real-time air purification rate in m³/h (speed-dependent)
- **Filter Life Left**: Percentage of filter lifetime remaining (0-100%)

### Binary Sensors
- **Filter Low**: Indicates when filter life drops below 5%

### Buttons
- **Reset Filter Stats**: Resets used CADR and runtime tracking, clears filter life calculation

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
      url: https://github.com/tuct/esphome-projects
      ref: main
    components: [levoit]
```

#### Step 2: Set esp32 variant

For Core 300/400s and Levoit 100s/200s:

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_FREERTOS_UNICORE: y

for custom esp, set accordingly 


#### Step 2: Configure UART
Match your hardware connections:
```yaml
uart:
  tx_pin: GPIO17   # ESP TX → MCU RX for original ESP32!
  rx_pin: GPIO16   # ESP RX → MCU TX for original ESP32!
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
├── levoit_message.h/.cpp   # Message building and frame construction utilities
├── decoder.cpp/.h          # Frame parsing and message dispatch
├── types.h                 # Enum definitions for commands and entity types
├── core_status.cpp/.h      # Core series status/timer payload decoders
├── vital_status.cpp/.h     # Vital series status payload decoders
├── core_commands.cpp/.h    # Core series command builders (300S/400S)
├── vital_commands.cpp/.h   # Vital series command builders (100S/200S)
├── decoder_helpers.h       # Shared utility functions
├── tlv.cpp/.h             # TLV encoding for complex payloads
└── [platform]/            # ESPHome platform integrations
    ├── fan/               # Fan entity
    ├── switch/            # Switch entities (display, lock, etc.)
    ├── number/            # Number entities (timer, room size)
    ├── select/            # Select entities (auto mode)
    ├── sensor/            # Sensor entities (PM2.5, CADR, filter life)
    ├── binary_sensor/     # Binary sensor entities (filter low status)
    ├── button/            # Button entities (filter reset)
    └── text_sensor/       # Text sensor entities (timer display)
```

### Protocol Details
- **Interface**: UART at 115200 baud
- **Frame Format**: `A5 [type:3] [seq:1] [len:1] [reserved:1] [chk:1] [cmd:1] [payload:N]`
- **Byte Order**: Little-endian for multi-byte values
- **Message Counter**: Global sequence number (`messageUpCounter`) tracks outgoing messages
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

sensor:
  - platform: levoit
    levoit_id: air_purifier
    current_cadr:
      name: "Current CADR"
    filter_life_left:
      name: "Filter Life Left"

binary_sensor:
  - platform: levoit
    levoit_id: air_purifier
    type: filter_low
    name: "Filter Low"

button:
  - platform: levoit
    levoit_id: air_purifier
    type: reset_filter_stats
    name: "Reset Filter Stats"
```

For complete configuration examples, see the [free-levoit project](../../projects/free-levoit/).

## Filter Life Calculation

The **Filter Life Left** sensor tracks filter usage as a percentage (0-100%) based on cumulative CADR consumption.

### How It Works

1. **Baseline Capacity**:
   - Model CADR (e.g., 214 m³/h for Core300S) multiplied by filter lifetime (default 12 months)
   - Formula: `Total Capacity = CADR × 24 hours × 30 days × Filter Lifetime (months)`
   - Example: `214 × 24 × 30 × 12 = 1,844,160 m³` for Core300S at 12 months

2. **Real-Time Tracking**:
   - Every minute, the component accumulates CADR based on current fan speed
   - Tracks `used_cadr_` (total m³ processed) persisted to device preferences
   - Updates `total_runtime_` (minutes fan has been on)

3. **Speed-Dependent CADR**:
   - Level 1: `CADR × 1 ÷ max_speed` (derates to ~63% in Sleep mode)
   - Level 2: `CADR × 2 ÷ max_speed`
   - Level 3: `CADR × 3 ÷ max_speed`
   - Level 4: `CADR × 4 ÷ max_speed` (Core400S/Vital series only)

4. **Percentage Calculation**:
   ```
   Filter Life % = 100 - (used_cadr ÷ Total Capacity × 100)
   ```
   - Clamped to 0-100% range
   - Published every 10 seconds as a float with one decimal place

5. **Binary Sensor Threshold**:
   - **Filter Low** binary sensor activates when `Filter Life % < 5%`
   - Useful for automations (e.g., order replacement reminders)

### Resetting Filter Stats

- Use the **Reset Filter Stats** button to reset `used_cadr` and `total_runtime` to 0
- This resets the filter life percentage back to 100%
- Persists immediately to device storage

### Configuration

Adjust filter lifetime expectancy via the **Filter Lifetime (months)** number entity:
```yaml
number:
  - platform: levoit
    levoit_id: air_purifier
    filter_lifetime_months:
      name: "Filter Lifetime (months)"
      min_value: 1
      max_value: 24
      step: 1
```

Default: 12 months. Adjust based on your filter's actual rated lifespan or usage pattern.

## Development Notes

### Code Organization
- **Message Building**: Centralized in `levoit_message.h/.cpp` with inline functions for efficiency
- **Command Builders**: Separated into `core_commands.cpp` and `vital_commands.cpp` for model-specific logic
- **Global Counter**: `messageUpCounter` tracks outgoing message sequence (inline variable in `levoit_message.h`)

### Adding New Commands
1. Add enum to `CommandType` in [types.h](types.h)
2. Implement in `build_core_command()` or `build_vital_command()` depending on model series
3. Pattern: Define `msg_type` and `payload` vectors, return `build_levoit_message(msg_type, payload, messageUpCounter)`
4. The `build_levoit_message()` function handles frame construction, counter insertion, and checksum calculation

### Debugging
Enable verbose logging in your YAML:
```yaml
logger:
  level: VERBOSE
  
uart:
  debug:
    direction: BOTH  # Monitor raw UART traffic
```

## ESPHome 2025.12.5+ Compatibility

This component has been updated for ESPHome 2025.12.5 with the following changes:

### Fan Component Updates
- **Preset Mode API**: Now uses internal tracking (`current_preset_`) since `preset_mode_` is private
- **Preset Modes**: Changed from `std::set` to `std::vector<std::string>` for `supported_preset_modes()`
- **FanCall Handling**: Updated to use `has_preset_mode()` and proper string comparisons

### Text Sensor Icons
- Icons now configured via Python codegen schema rather than C++ `setup()`
- Ensures proper Home Assistant entity registration with correct icons

### Select Component
- Migrated from deprecated `.state` to `current_option()` method

## Known Issues & TODO
- [ ] Implement custom sleep mode settings for Vital series
- [ ] Model-specific room size validation based on CADR ratings
- [ ] Verify Core 300S/400S protocol differences (MCU version dependency)
- [x] WiFi LED control and status indication after connection
- [x] Add filter life time and current CADR sensors
- [x] Add filter low binary sensor (< 5% threshold)
- [x] Add reset filter stats button
- [x] Enable filter reset from Home Assistant
- [x] Enable filter reset from Device (Long press sleep)
- [x] Add appropriate icons for entities
- [x] Test compatibility with ESPHome 2025.12+ (preset mode API changes)
- [x] Update to ESPHome 2025.12.5 (completed)


## Credits

Special thanks to the original developers who reverse-engineered the Levoit protocols:
- [targor](https://github.com/targor/) - [Levoit Vital integration](https://github.com/targor/levoit_vital/)
- [mulcmu](https://github.com/mulcmu/) - [Levoit Core 300S](https://github.com/mulcmu/esphome-levoit-core300s)
- [acvigue](https://github.com/acvigue/) - [Levoit Core integration](https://github.com/acvigue/esphome-levoit-air-purifier)

## License

This component is provided as-is for educational and personal use. Levoit and related trademarks are property of their respective owners.

