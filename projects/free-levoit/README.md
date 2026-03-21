[← Back to Projects](../README.md)

# Project - Free Levoit Air Purifiers

Collection of custom ESPHome firmware and hardware projects for Levoit air purifiers, eliminating cloud dependency and enabling native Home Assistant integration.

Here is a list of all 5 models running with ESPHome. Core 300S and Core 400S only require disassembly to flash firmware (no hardware modifications needed). The Mini and LV-PUR 131S require custom hardware hacks and PCB modifications.

* [Generic Levoit ESPHome Component](../../components/levoit/), full custom ESPHome-based firmware

* [levoit-core200s](./levoit-core200s) – Custom ESPHome Firmware - WIP!
* [levoit-core300s](./levoit-core300s) – Custom ESPHome Firmware
* [levoit-core400s](./levoit-core400s) – Custom ESPHome Firmware
* [levoit-vital100s](./levoit-vital100s) – Custom ESPHome Firmware
* [levoit-lv-pur131s](./levoit-lv131s/) – Custom Firmware + MCU & sensor upgrade + hardware hack
* [levoit-mini](./levoit-mini) – Custom PCB, 3D parts, hardware hack

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



# Levoit MCU ⇄ ESP (Core 200/300/400, Vital 100/200)

UART protocol between the Levoit MCU and the ESP32. Core and Vital share the same framing, but payload layouts differ (Core = fixed fields, Vital = TLV blocks).

Data is aggregated from the community and aligned with the current ESPHome component implementation.

Thanks to

- https://github.com/mulcmu/
- https://github.com/acvigue/
- https://github.com/targor/

Main reference: [protocol sheet](https://docs.google.com/spreadsheets/d/1vSKuPWYplPWVsKXIuISHjPhHEBLqe_wB/edit?usp=sharing&ouid=109862782260450893370&rtpof=true&sd=true) (also [local snapshot](./Levoit%20Uart%20Communication.xlsx)).

## Frame Overview
- Transport: UART 115200 8N1
- Start: `0xA5`
- Message type: `0x22` (send), `0x12` (ack), `0x52` (error)
- Counter: 1 byte at index 2 (starts at 0x10 in code, increments per packet)
- Length: byte 3 = bytes after checksum (index 5)
- Checksum: byte 5 = `0xFF - (sum(all bytes except checksum) & 0xFF)` (see `levoit_checksum()` / `levoit_finalize_message()`)
- Payload type: indexes 6-8 (model-specific)
- Reserved: byte 4 = 0, byte 9 = 0
- Command size: byte 6 meaning depends on model (Core uses 0x01 2-byte cmd; Vital can use 0x02 and extends with bytes 10/11)

### Core Payload Types (examples)
- Status response: `0x01 0x30 0x40` (Core300/400)
- Status request: `0x01 0x31 0x40`
- Timer status: `0x01 0x65 0xA2`

### Vital Payload Types
- Status response: `0x02 0x00 0x55` (TLV list follows)

### Payload Layouts
- **Core**: fixed fields (power, mode, speed, display, PM2.5, auto-mode config). See per-model tables below.
- **Vital**: TLV blocks (id, len, value). See table below for known IDs.

### Building Messages (from code)
- Helper: `build_levoit_message(msg_type, payload, counter)` sets start byte, message type, counter, length, checksum.
- Counter and checksum logic live in `levoit_message.h` (`levoit_checksum`, `levoit_finalize_message`).
- Model-specific command builders: `build_core_command()` and `build_vital_command()` assemble payloads and types.


## Header Formats

- Byte 0: `0xA5` start
- Byte 1: message type (`0x22` send, `0x12` ack, `0x52` error)
- Byte 2: counter (set by ESP, echoed by MCU) — starts at 0x10 in code, increments per packet
- Byte 3: length = bytes after checksum (index 5)
- Byte 4: reserved `0x00`
- Byte 5: checksum = `0xFF - (sum(all bytes except checksum) & 0xFF)` (see `levoit_checksum`)
- Bytes 6-8: payload type / command id (model-specific)
- Byte 9: reserved `0x00`
- Byte 10: payload type extension or first payload byte (Vital uses 0x02 with extended type at byte 10)

**Payload starts at byte 11** (after 10-byte header for Core; Vital may vary with TLV structure).

### Command/Payload Type Encoding

**Core Series**: Fixed-field payloads
- Command size code `0x01` (2-byte command id)
- Payload after header contains fixed position fields (power, mode, speed, display, PM2.5, etc.)
- No TLV encoding

**Vital Series**: TLV (Type-Length-Value) encoding
- Command size code `0x02` (extended command id with bytes 10+)
- **All responses use TLV blocks**: each block has id (1 byte), length (1 byte), value (1-N bytes)
- **Most commands use TLV**: commands also send data as TLV blocks (e.g., set auto mode, filter settings)
- Allows flexible extensibility and model variants without protocol redesign

Examples:
- Core status response: `01 30 40` → fixed 18-byte payload
- Core status request: `01 31 40` → 1-byte request
- Core timer status: `01 65 A2` → variable timer payload
- Vital status response: `02 00 55` → TLV blocks follow (id/len/value repeating)

Message builder in code: `build_levoit_message(msg_type, payload, counter)` handles start, length, counter, checksum automatically.

## Commands

All protocol details can be found [here](./Levoit%20Uart%20Communication.xlsx)

Core and Vital have different command structures; both use the headers described above.

### Core Series

![core commands](./commands_core.png)

### Vital Series

![vital commands](./commands_vital.png)
![vital sleep config commands](./commands_vital_sleep_conf.png)

## Responses

|Devices|Type|Start| Message Type  |  Sequence | Payload Size  |  Zero | Checksum  | CMD Size | CMD  |  CMD | Zero ||||||||||
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
||| B.0 | B.1 | B.2 | B.3 | B.4 | B.5 |B.6 | B.7 | B.8 |B.9| B.10 |B.11 |B.12 |B.13 | B.14 | B.15 |B.16| B.17 ||
|Core200/300s| Timer running| 0xA5 | 0x22  | 0xXX  | 0x0C | 0x00  | 0xXX  |  **0x01**	|**0x65**	|**0xA2**	|0x00|**0x08**	|**0x0D**	|0x00	|0x00	|**0x10**	|**0x0E**	|0x00|	0x00|	B10/11 = Remaining time & B14/15 >initial timer value int 16|
|Core200/300s| Filter reset pressed| 0xA5 | 0x22  | 0xXX  | 0x05 | 0x00  | 0xXX  |**0x01**|	**0xE4**|**0xA5**|	0x00| During Timer running|



### Status Responses



|Devices|Start| Message Type  |  Sequence | Payload Size  |  Zero | Checksum  | CMD Size | CMD  |  CMD | Zero  | CMD<br>Payload |  CMD<br>Payload|  
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|| B.0 | B.1 | B.2 | B.3 | B.4 | B.5 |B.6 | B.7 | B.8 |B.9| B.10 |B.11 |
|Core200/300s| 0xA5 | 0x22  | 0xXX  | 0x16 | 0x00  | 0xXX  |  **0x01** | **0x1B** | **0x40** |0x00|||
|Core400s| 0xA5 | 0x22  | 0xXX  | 0x16  | 0x00  | 0xXX  | **0x01** | **0x30**	| **0x40** |0x00|||
|Vital 100/200s| 0xA5 | 0x22  | 0xXX  | 0x6C | 0x00  | 0xXX  |  **0x02** | **0x00**	| **0x55** | 0x00 | **0x00** | **0x01** |




### Response format for Core versions

Response payload for core versions has a max lenght of 28 bytes total, 10 header, 18 max payload.

Response starts at byte 10, zero on bytes 19 and 27. 

|Byte|Core 200/300s<br>0x01 0x1B 0x40|Description|Core 400s - Difference<br>0x01 0x30 0x40|Core 400s - values|
|:---|:--------|:----------------|:-------------------|:----------------|
|B.10|HW Version patch||| |
|B.11|HW Version minor||| |
|B.12|HW Version major||| |
|B.13|Power|00 Off<br>01 On|| |
|B.14|Fan Mode|00 Manual<br>01 Sleep<br>02 Auto|| |
|B.15|Manual Fan Speed Selected|00<br>01 Low<br>02 Med<br>03 High|Current Fan Speed|00 Sleep<br>01 Low<br>02 Med<br>03 High<br>04 Highest<br>255 Power Off|
|B.16|Screen Brightness|00 Screen Off<br>64 Screen Full|Manual Fan Speed Selected|00<br>01 Low<br>02 Med<br>03 High<br>04 Highest|
|B.17|Screen|00 Off<br>01 On|Screen Brightness|00 Screen Off<br>64 Screen Full|
|B.18|Current Fan Speed|00 Sleep<br>01 Low<br>02 Med<br>03 High<br>255 Power Off|Screen|00 Off<br>01 On|
|B.19|Always Zero||| |
|B.20|PM2.5 AQI|01 - 04|| |
|B.21 B.22|PM2.5|int16 pm2.5 value|| |
|B.23|Display Lock|00 Unlocked<br>01 Locked|| |
|B.24|Fan Auto Mode|00 Default<br>01 Quiet<br>02 Efficient|| |
|B.25 B.26|Efficient Area|3B 01 100 sq ft (App Minimum) 0x013B is 315<br>EC 04 400 sq ft (App Maximum) 0x04EC is 1,260|| |
|B.27|Zero Or One||| |




### Response format for Vital versions

Response payload for vital versions has a max lenght of 114 bytes total, 12 header, 102 max payload.

Response starts at byte 13,
Each part of the response has an ID, 2 bytes => 0x04 0x01, first byte is the id, 2nd the length of data 0x01 = 1 byte and 0x02 = 2 bytes.

|Data ID| Data Size  | Data Payload |Data ID| Data Size  |  Data Payload | Data Payload |  ... |
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| B.10 | B.11  | B.12  | B.13| B.14  | B.15 |B.16 | B.XX |
| **0x00**| **0x01** | 0xXX  | **0x01**| **0x02** | 0xXX |0xXX | ... |




Here is a table with all know data from the vital 200s (TLV format)

| TLV ID | Type   | Parameter | Description | ESPHome Entity |
|--------|--------|-----------|------------|-----------------|
| 0x00   | u32    | Device ID | Device identification | - |
| 0x01   | 3xU8   | MCU Version | major.minor.patch | TextSensor: MCU_VERSION |
| 0x02   | u32    | Power | 0 Off, 1 On | Fan power state |
| 0x03   | u32    | FAN MODE | 0 Manual, 1 Auto, 2 Sleep, 5 PET mode | Fan mode |
| 0x04   | u32    | FAN LEVEL | 0 minimum, 1, 2, 3, 4 (circulation), 5 auto | Fan speed |
| 0x05   | u32    | FAN SPEED | Alternative speed representation | - |
| 0x06   | u32    | Display Illuminated | 0 Off, 1 On | Switch: DISPLAY |
| 0x07   | u32    | Display State | Display on/off state | - |
| 0x08   | u32    | Unknown_08 | (not yet documented) | - |
| 0x09   | u32    | Air Quality Level (AQI) | 1 - 4 | Sensor: AQI |
| 0x0A   | u32    | Air Quality Detail | 0 Sensor Error, other Ok | TextSensor: ERROR_MESSAGE |
| 0x0B   | u32    | PM2.5 | Particulate density | Sensor: PM25 |
| 0x0E   | u32    | Child Lock | 0 Unlocked, 1 Locked | Switch: CHILD_LOCK |
| 0x0F   | u32    | Auto Mode | 0 Default, 1 Quiet, 2 Efficient | Select: AUTO_MODE |
| 0x10   | u32    | Efficiency Room Size | Room size value (100-1800) | Number: EFFICIENCY_ROOM_SIZE |
| 0x11   | u32    | Efficiency Counter | Time counter for efficiency mode | Sensor: EFFICIENCY_COUNTER |
| 0x12   | u32    | Auto Mode Profile | (not yet documented) | - |
| 0x13   | u32    | Light Detect | 0 Off, 1 On | Switch: LIGHT_DETECT |
| 0x16   | u32    | Wifi Light | Wifi LED state | - |
| 0x17   | u32    | Dark Detected | Dark/light detection | - |
| 0x18   | u32    | Sleep Mode Type | Sleep mode configuration | - |
| 0x19   | u32    | Quick Clean Enabled | 0/1 enable flag | - |
| 0x1A   | u32    | Quick Clean Minutes | Duration in minutes | - |
| 0x1B   | u32    | Quick Clean Fan Level | 1-4 | - |
| 0x1C   | u32    | White Noise Enabled | 0/1 enable flag | - |
| 0x1D   | u32    | White Noise Minutes | Duration in minutes | - |
| 0x1E   | u32    | White Noise Fan Level | 1-4 | - |
| 0x1F   | u32    | Sleep Fan Mode/Level | Sleep mode speed setting | - |
| 0x20   | u32    | Sleep Mode Minutes | Sleep mode duration | - |
| 0x21   | u32    | Daytime Enabled | 0/1 daytime schedule | - |
| 0x22   | u32    | Daytime Fan Mode | Fan mode for daytime | - |
| 0x23   | u32    | Daytime Fan Level | Fan speed for daytime | - |




# More old stuff
Just so i don't loose it!

## HW Details

Different FW! 3.0.0

Main changes 
* Message identifier for status changed from 01 30 40 to 01 B0 40
* Position of fan speed and display on changed compared to 300s other message ids!

01 B0 40 - Status (MCU to ESP)

Header new 

A5 22 F7 16 00 10 **01 B0 40** 00 00 00 03 

Header Old

A5 22 1D 16 00 E4 **01 30 40** 00 07 00 02 #

Payload New

01 00 01 01 64 01 00 01 FF FF 00 02 BD 00 01

Payload OLD

01 00 01 64 01 00 00 01 03 00 00 00 3B 01 00


22 byte long (0x16) status packet payload:


### Header - 10 bytes long, 0 after 4 bytes!

- Byte 1 A5 start byte of packet
- Byte 2 22 send message or 12 ack message (52 might be error response)
- Byte 3 1-byte sequence number, increments by one each packet
- Byte 4 1-byte size of payload (after checksum byte)
- Byte 5 Always 0
- Byte 6 1-byte checksum. Computed as 255 - ( (sum all of bytes except checksum) % 256 )
- Byte 7 Always Payload Type /Version
- Byte 8 Always Payload Type /Version
- Byte 9 Always Payload Type /Version
- Byte 10 Always 0

bytes 11 + payload (size includes bytes 7-10 => min value0x04)


Bytes 7-9 payload type/ cmd

- case 0x013040:  //Status response core300S
- case 0x01B040:  //status response core400S
- case 0x020055:  //Status response Vital 200s/pro

- case 0x013140:  //status request core300s
```
enum class LevoitDeviceModel : uint8_t { NONE, CORE_200S, CORE_300S, CORE_400S };
enum class LevoitPacketType : uint8_t { SEND_MESSAGE = 0x22, ACK_MESSAGE = 0x12, ERROR = 0x52 };
enum class LevoitPayloadType : uint32_t {
  STATUS_REQUEST =        0x01 31 40,
  STATUS_RESPONSE =       0x01 30 40,
  AUTO_STATUS =           0x01 60 40, // I only know this value for 200S, so might be wrong  
  SET_FAN_AUTO_MODE =     0x01 E6 A5,
  SET_FAN_MANUAL =        0x01 60 A2,
  SET_FAN_MODE =          0x01 E0 A5,
  SET_DISPLAY_LOCK =      0x01 00 D1,
  SET_WIFI_STATUS_LED =   0x01 29 A1,
  SET_POWER_STATE =       0x01 00 A0,
  SET_SCREEN_BRIGHTNESS = 0x01 05 A1,
  SET_FILTER_LED =        0x01 E2 A5,
  SET_RESET_FILTER =      0x01 E4 A5,
  TIMER_STATUS =          0x01 65 A2,
  SET_TIMER_TIME =        0x01 64 A2,
  TIMER_START_OR_CLEAR =  0x01 66 A2,
  SET_NIGHTLIGHT =        0x01 03 A0
};

enum class LevoitState : uint32_t {
  POWER               = 1 << 0,
  FAN_MANUAL          = 1 << 1,
  FAN_AUTO            = 1 << 2,
  FAN_SLEEP           = 1 << 3,
  DISPLAY             = 1 << 4,
  DISPLAY_LOCK        = 1 << 5,
  FAN_SPEED1          = 1 << 6,
  FAN_SPEED2          = 1 << 7,
  FAN_SPEED3          = 1 << 8,
  FAN_SPEED4          = 1 << 9,
  NIGHTLIGHT_OFF      = 1 << 10,
  NIGHTLIGHT_LOW      = 1 << 11,
  NIGHTLIGHT_HIGH     = 1 << 12,
  AUTO_DEFAULT        = 1 << 13,
  AUTO_QUIET          = 1 << 14,
  AUTO_EFFICIENT      = 1 << 15,
  AIR_QUALITY_CHANGE  = 1 << 16,
  PM25_NAN            = 1 << 17,
  PM25_CHANGE         = 1 << 18,
  WIFI_CONNECTED      = 1 << 19,
  HA_CONNECTED        = 1 << 20,
  FILTER_RESET        = 1 << 21,
  WIFI_LIGHT_SOLID    = 1 << 22,
  WIFI_LIGHT_FLASH    = 1 << 23,
  WIFI_LIGHT_OFF      = 1 << 24
};
```





### Payload New:

Byte 11 HW Version Minor

Byte 12 HW Version Major

Byte 13 Power:

* 00 Off
* 01 On

Byte 14 Fan mode:

* 00 Manual
* 01 Sleep
* 02 Auto



Byte 15 Current Fan Speed => diff from core300s

* 00 Sleep
* 01 Low
* 02 Med
* 03 High
* 255 Power Off

Byte 16 Manual Fan Speed Selected => diff from core300s

* 00 (Occurs at startup for 1 packet)
* 01 Low
* 02 Med
* 03 High

Byte 17 Screen Brightness => diff from core300s

* 00 Screen Off
* 64 Screen Full (Screen illuminates briefly when another button is tapped while screen is off)

Byte 18 Screen

* 00 Off
* 01 On


Byte 19 Always 0

Byte 20 PM2.5 (Normally 1 increased to 4 during filter testing, color ring LEDs)

Byte 21 & 22 PM2.5 0xFFFF when off, samples every 15 minutes when off

Byte 23 Display Lock:

* 00 Unlocked
* 01 Locked

Byte 24 Fan Auto Mode: (Only configurable by the app)

* 00 Default, speed based on air quality
* 01 Quiet, air quality but no max speed
* 02 Efficient, based on room size

Byte 25 & 26 Efficient Area:

3B 01 100 sq ft (App Minimum) 0x013B is 315

EC 04 400 sq ft (App Maximum) 0x04EC is 1,260

Linear scale, not sure what the units are.

Byte 27 Always 0


## Commands

### 01 E6 A5 - Configure Fan Auto Mode (ESP to MCU)

Byte 4 Always 0

Byte 5 Fan Auto Mode

- 00 Default, speed based on air quality
- 01 Quiet, air quality but no max speed
- 02 Efficient, based on room size
- Byte 6 & 7 00 00 or Efficient Area

### 01 60 A2 - Set Fan Manual (ESP to MCU)

Byte 4 Always 0

Byte 5 Always 0

Byte 6 Always 1

Byte 7 Fan Speed:

- 01 Low
- 02 Med
- 03 High
- 04 Highest 

### 01 E0 A5 - Set Fan Mode (ESP to MCU)

Byte 4 Always 0

Byte 5 Fan Mode

- 00 Manual (App always uses 01 60 A2 with speed to change to manual mode)
- 01 Sleep
- 02 Auto

### 1 0 D1 - Display Lock (ESP to MCU)

Byte 4 Always 0

Byte 5 Display Lock:

- 00 Unlocked
- 01 Locked
- 01 29 A1 - Wifi LED state (ESP to MCU)

### Wifi LED toggled at startup and when network connection changes

1	29	A1	0	0	F4	1	F4	1	0 Off

1	29	A1	0	1	7D	0	7D	0	0 On

1	29	A1  0   2   F4  1   F4  1   0 Blink

### 01 31 40 - Request Status (ESP to MCU)

Similar to status packet, occurs when Wifi led state is changed.

### 01 00 A0 - Set Power State (ESP to MCU)

Byte 4 Always 0

Byte 5 Fan Speed:

- 00 Off
- 01 On

### 01 05 A1 Set Brightness (ESP to MCU)

Byte 4 Always 0

Byte 5 Screen Brightness

00 Screen Off
64 Screen Full

### 01 E2 A5 - Set Filter LED (ESP to MCU)

Byte 4

- 00 Off
- 01 On

Byte 5 0

Wasn't in original captures before firmware update

### 01 E4 A5 - Reset Filter (ESP to MCU and MCU to ESP)
Byte 4 0

Byte 5 0

MCU sends to ESP when sleep button held for 3 seconds

ESP sends to MCU when reset on app.

### 01 65 A2 - Timer Status (MCU to ESP)

MCU sends a packet when timer is running with remaining time

A5 12 27 0C 00 DA 01 65 A2 00 08 0D 00 00 10 0E 00 00

Remaining time and initial time.

0x0D08 remaining seconds

0x0E10 initial seconds

### 1 64 A2 Set Timer Time (ESP to MCU)

Byte 4 Always 0

Byte 5 & 6 Time

Byte 7 & 8 Always 0

### 1 66 A2 Timer Start or Clear (MCU to ESP)

Byte 4 to 12 All 0 to Clear

Byte 5 & 6 and Byte 9 &10 set to same initial timer value




