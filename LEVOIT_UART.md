# Levoit UART Protocol

UART protocol between the Levoit MCU and the ESP32, as implemented by the ESPHome component.

All models share the same frame format. Payload structure differs by series:

| Series | Models | Payload format | CMD family |
|--------|--------|----------------|------------|
| **Core** | Core 200S, 300S, 400S, 600S | Fixed-field (byte offsets) | cmd[0] = `0x01` |
| **Vital** | Vital 100S, 200S (Pro) | Flat TLV (tag/len/value) | cmd[0] = `0x02` |
| **Sprout** | Sprout | Vital TLV base + Sprout extensions | cmd[0] = `0x02` |

The Sprout uses the same flat-TLV protocol as Vital (`CMD=02 00 55` for status, `CMD=02 02/03/04 55` for fan control) with additional Sprout-specific CMDs for the LED ring, white noise, CO₂/VOC sensors, and MCU async events.

Data aggregated from the community and aligned with current component implementation.

Thanks to [mulcmu](https://github.com/mulcmu/), [acvigue](https://github.com/acvigue/), [targor](https://github.com/targor/).

Main reference: [protocol sheet](https://docs.google.com/spreadsheets/d/1vSKuPWYplPWVsKXIuISHjPhHEBLqe_wB/edit?usp=sharing&ouid=109862782260450893370&rtpof=true&sd=true) (also [local snapshot](./Levoit%20Uart%20Communication.xlsx)).

---

## Transport

- **Baud rate:** 115200
- **Format:** 8N1

---

## Frame Format

```
[0]  0xA5          start byte
[1]  msg_type      0x22=SEND  0x12=ACK  0x52=ACK (Core600S / Sprout)
[2]  counter       increments per sent packet, starts at 0x10; MCU echoes ESP's counter
[3]  length        number of bytes after the checksum byte: msg.size() - 6
[4]  0x00          reserved
[5]  checksum      0xFF - (sum of all bytes except [5]) & 0xFF
[6]  cmd[0]        \
[7]  cmd[1]         > command / payload type (3 bytes, model-specific)
[8]  cmd[2]        /
[9]  0x00          reserved
[10+] payload      starts here
```

**Checksum** (from `levoit_checksum()`): sum all bytes except byte[5], take `0xFF - (sum & 0xFF)`.

**Counter** starts at `0x10` (16). ESP increments per outgoing packet; MCU echoes the same counter value in its ACK.

---

## Message Types

| Value | Name | Direction | Notes |
|-------|------|-----------|-------|
| `0x22` | SEND | both | Normal message carrying payload |
| `0x12` | ACK  | both | Acknowledgement, minimal or no payload |
| `0x52` | ACK  | MCU→ESP | Core600S and Sprout — replaces `0x12` with trailing `0x01` |

### `0x22` CMD Families

The first byte of the three CMD bytes (frame bytes [6][7][8]) identifies the protocol family:

**Core series — cmd[0] = `0x01`**

The second byte identifies the subsystem, the third byte identifies the direction/category:

| cmd[1] | cmd[2] | Description |
|--------|--------|-------------|
| `0x30` / `0xB0` / `0x60` / `0x40` | `0x40` / `0x41` | Status push (MCU→ESP) — fixed-field payload |
| `0x00` | `0xA0` | Power on/off |
| `0x60` | `0xA2` | Set fan speed |
| `0xE0` | `0xA5` | Set fan mode |
| `0xE6` | `0xA5` | Set auto mode / room size |
| `0x05` | `0xA1` | Set display brightness |
| `0x00` | `0xD1` | Set child lock |
| `0x29` | `0xA1` | Set Wi-Fi LED |
| `0xE9` | `0xA5` | Set light detect (Core600S) |
| `0x03` | `0xA0` | Set nightlight (Core200S) |
| `0xE2` | `0xA5` | Set filter LED |
| `0x64` / `0x65` / `0x66` | `0xA2` | Timer (set / request / update) |

**Vital / Sprout series — cmd[0] = `0x02`**

The second byte identifies the function, the third byte is always `0x50` (control/timer) or `0x55` (status/feature):

| cmd[1] | cmd[2] | Description |
|--------|--------|-------------|
| `0x00` | `0x55` | Status push (MCU→ESP) — flat TLV payload |
| `0x00` | `0x50` | Power on/off |
| `0x02` | `0x55` | Set fan mode / auto mode |
| `0x03` | `0x55` | Set fan speed |
| `0x04` | `0x55` | Set display |
| `0x11` | `0x55` | Set light detect |
| `0x40` | `0x51` | Set child lock |
| `0x18` | `0x50` | Set Wi-Fi LED |
| `0x19` | `0x50` | Set timer |
| `0x1A` | `0x50` | Request timer status |
| `0x1B` | `0x50` | Timer update pushed (MCU→ESP) |
| `0x08` | `0x55` | Sprout: async event (MCU→ESP) |
| `0x0B` | `0x55` | Sprout: set nightlight |
| `0x0C` | `0x55` | Sprout: set breathing |
| `0x07` | `0x55` | Sprout: set white noise |
| `0x06` | `0x55` | Sprout: set AQI scale |

---

## Command IDs (Status / Inbound from MCU)

| Model | CMD bytes | msg_type | Notes |
|-------|-----------|----------|-------|
| Core 200S | `01 60 40` | `0x22` | MCU pushes status |
| Core 300S | `01 30 40` | `0x22` | MCU pushes status |
| Core 300S (FW ≥ 3.x) | `01 B0 40` | `0x22` | Newer MCU firmware |
| Core 400S | `01 30 40` / `01 B0 40` | `0x22` | Same as 300S |
| Core 600S | `01 40 41` | `0x22` | MCU pushes status |
| Vital 100/200S, Sprout | `02 00 55` | `0x22` | Flat-TLV payload |

Timer responses (MCU→ESP):

| Series | CMD bytes | msg_type | Notes |
|--------|-----------|----------|-------|
| Core | `01 65 A2` | `0x12` | Timer status ACK |
| Core | `01 66 A2` | `0x22` | Timer update pushed |
| Vital/Sprout | `02 1A 50` | `0x12` | Timer status ACK |
| Vital/Sprout | `02 1B 50` | `0x22` | Timer update pushed |

Sprout async events (MCU→ESP):

| CMD bytes | msg_type | Notes |
|-----------|----------|-------|
| `02 08 55` | `0x22` | Button events, white noise state, cover sensor |

---

## Core Status Payload (Fixed Fields)

Payload starts at frame byte **[10]** (offset 0 in payload arrays below).

### Common header (all Core models)

| Offset | Frame byte | Field | Values |
|--------|------------|-------|--------|
| 0 | B.10 | MCU version patch | — |
| 1 | B.11 | MCU version minor | — |
| 2 | B.12 | MCU version major | — |
| 3 | B.13 | Power | `0x00`=off, `0x01`=on |
| 4 | B.14 | Fan mode | `0x00`=Manual, `0x01`=Sleep, `0x02`=Auto |

### Core 200S — CMD `01 60 40`

| Offset | Frame byte | Field | Values |
|--------|------------|-------|--------|
| 5 | B.15 | Fan speed | `0x01`–`0x03` |
| 6 | B.16 | Display | `0x00`=off, non-zero=on |
| 10 | B.20 | Child lock | `0x00`=off, `0x01`=on |
| 11 | B.21 | Nightlight | `0x00`=off, `0x32`=mid, `0x64`=full |

### Core 300S — CMD `01 30 40` / `01 B0 40`

| Offset | Frame byte | Field | Values |
|--------|------------|-------|--------|
| 5 | B.15 | Fan speed | `0x01`–`0x03` |
| 6 | B.16 | Display | `0x00`=off, non-zero=on |
| 10 | B.20 | AQI | `0x01`–`0x04` |
| 11–12 | B.21–B.22 | PM2.5 | LE16 µg/m³ |
| 13 | B.23 | Child lock | `0x00`=off, `0x01`=on |
| 14 | B.24 | Auto mode | `0x00`=Default, `0x01`=Quiet, `0x02`=Room Size |
| 15–16 | B.25–B.26 | Efficiency area | LE16 raw (see [Room Size Encoding](#room-size-encoding)) |
| 17 | B.27 | Error | `0x00`=ok, non-zero=sensor error |

### Core 400S — CMD `01 30 40` / `01 B0 40`

Same as Core 300S except:

| Offset | Frame byte | Field | Values |
|--------|------------|-------|--------|
| 5 | B.15 | Fan speed | `0x00`=sleep, `0x01`–`0x04`=speeds, `0xFF`=off |
| **7** | **B.17** | Display | `0x00`=off, non-zero=on *(offset differs from 300S)* |

### Core 600S — CMD `01 40 41`

| Offset | Frame byte | Field | Values |
|--------|------------|-------|--------|
| 5 | B.15 | Fan speed | `0x01`–`0x04` |
| 8 | B.18 | Display | `0x00`=off, non-zero=on |
| 11 | B.21 | AQI | `0x01`–`0x04` |
| 12–13 | B.22–B.23 | PM2.5 | LE16 µg/m³ |
| 14 | B.24 | Child lock | `0x00`=off, `0x01`=on |
| 15 | B.25 | Auto mode | `0x00`=Default, `0x01`=Quiet, `0x02`=Room Size, `0x03`=ECO |
| 16–17 | B.26–B.27 | Efficiency area | LE16 raw (see [Room Size Encoding](#room-size-encoding)) |
| 21 | B.31 | Light detect | `0x00`=off, `0x01`=on |

---

## Vital / Sprout Status TLV (CMD `02 00 55`)

Payload is a flat TLV sequence starting at frame byte **[10]**.
Each block: `[tag:1] [len:1] [value:len]`.

| TLV ID | Len | Field | Values | ESPHome Entity |
|--------|-----|-------|--------|----------------|
| `0x00` | 4 | Device ID | u32 identifier | — |
| `0x01` | 3 | MCU version | bytes: patch, minor, major | `mcu_version` |
| `0x02` | 1 | Power | `0x00`=off, `0x01`=on | fan power |
| `0x03` | 1 | Fan mode | `0x00`=Manual, `0x01`=Sleep, `0x02`=Auto, `0x05`=Pet | fan preset |
| `0x04` | 1 | Fan level | `0x00`=min … `0x04`=max | fan speed % |
| `0x05` | 1 | Fan speed (alt) | alternative speed representation | — |
| `0x06` | 1 | Display illuminated | `0x00`=off, `0x01`=on | `display` switch |
| `0x07` | 1 | Display state | display on/off state | — |
| `0x08` | 1 | Unknown | — | — |
| `0x09` | 1 | AQI | `0x01`–`0x04` | `aqi` sensor |
| `0x0A` | 1 | Air quality detail | `0x00`=sensor error, non-zero=ok | `error_message` |
| `0x0B` | 2 | PM2.5 | LE16 µg/m³ | `pm25` sensor |
| `0x0C` | 2 | PM1.0 raw | LE16 raw count *(Sprout only)* | `pm1_0` sensor |
| `0x0D` | 2 | PM10 raw | LE16 raw count *(Sprout only)* | `pm10` sensor |
| `0x0E` | 1 | Child lock | `0x00`=off, `0x01`=on | `child_lock` switch |
| `0x0F` | 1 | Auto mode | `0x00`=Default, `0x01`=Quiet, `0x02`=Efficient | `auto_mode` select |
| `0x10` | 2 | Efficiency room size | LE16 raw (see [Room Size Encoding](#room-size-encoding)) | `efficiency_room_size` number |
| `0x11` | 2 | Efficiency counter | LE16 seconds remaining at high speed | `efficiency_counter` sensor |
| `0x12` | 1 | Auto mode profile | — | — |
| `0x13` | 1 | Light detect | `0x00`=off, `0x01`=on | `light_detect` switch |
| `0x16` | 1 | Wi-Fi LED state | — | — |
| `0x17` | 1 | Dark detected | ambient light sensor | — |
| `0x18` | 1 | Sleep mode type | — | — |
| `0x19` | 1 | Quick clean enabled | `0x00`/`0x01` | — |
| `0x1A` | 1 | Quick clean minutes | duration | — |
| `0x1B` | 1 | Quick clean fan level | `0x01`–`0x04` | — |
| `0x1C` | 1 | White noise enabled | `0x00`/`0x01` | — |
| `0x1D` | 1 | White noise minutes | duration | — |
| `0x1E` | 1 | White noise fan level | `0x01`–`0x04` | — |
| `0x1F` | 1 | Sleep fan level | — | — |
| `0x20` | 1 | Sleep mode minutes | — | — |
| `0x21` | 1 | Daytime enabled | `0x00`/`0x01` | — |
| `0x22` | 1 | Daytime fan mode | — | — |
| `0x23` | 1 | Daytime fan level | `0x01`–`0x04` | — |
| `0x24` | 4 | Nightlight state *(Sprout)* | `{on, brightness_pct, ct_lo, ct_hi}` — brightness 0–100, color temp Kelvin LE16 | `light` |
| `0x25` | 6 | Breathing state *(Sprout)* | `{mode, speed_sec, ct_lo, ct_hi, min_pct, max_pct}` — mode `0x01`=breathing; cycle 1–10 s; CT Kelvin LE16; brightness 0–100 | `light` |
| `0x26` | 1 | Breathing active *(Sprout)* | `0x01`=breathing running, `0x00`=off | — |
| `0x27` | 2 | Fan RPM *(Sprout)* | LE16 tachometer reading | `fan_rpm` sensor |

---

## Core Commands (ESP→MCU)

| Command | CMD bytes | Payload | Notes |
|---------|-----------|---------|-------|
| Power on/off | `01 00 A0` | `{0x01}` / `{0x00}` | |
| Fan speed 1–4 | `01 60 A2` | `{0x01, 0x01, speed}` | speed = 1–4 |
| Fan mode | `01 E0 A5` | `{mode}` | 0=Manual, 1=Sleep, 2=Auto |
| Display on | `01 05 A1` | `{0x64}` | brightness full |
| Display off | `01 05 A1` | `{0x00}` | |
| Child lock on/off | `01 00 D1` | `{0x01}` / `{0x00}` | |
| Auto mode Default | `01 E6 A5` | `{0x00, 0x00, 0x00}` | |
| Auto mode Quiet | `01 E6 A5` | `{0x01, 0x00, 0x00}` | |
| Auto mode Room Size | `01 E6 A5` | `{0x02, size_lo, size_hi}` | see [Room Size Encoding](#room-size-encoding) |
| Auto mode ECO | `01 E6 A5` | `{0x03, 0x00, 0x00}` | Core600S only |
| Light detect on/off | `01 E9 A5` | `{0x01}` / `{0x00}` | Core600S only |
| Nightlight off | `01 03 A0` | `{0x00, 0x00}` | Core200S only |
| Nightlight mid | `01 03 A0` | `{0x00, 0x32}` | Core200S only |
| Nightlight full | `01 03 A0` | `{0x00, 0x64}` | Core200S only |
| Filter LED on/off | `01 E2 A5` | `{0x01}` / `{0x00}` | |
| Wi-Fi LED on | `01 29 A1` | `{0x01, 0x7D, 0x00, 0x7D, 0x00, 0x00}` | solid |
| Wi-Fi LED off | `01 29 A1` | `{0x00, 0xF4, 0x01, 0xF4, 0x01, 0x00}` | |
| Wi-Fi LED blink | `01 29 A1` | `{0x02, 0xF4, 0x01, 0xF4, 0x01, 0x00}` | connecting |
| Timer set | `01 64 A2` | `{sec_b0, sec_b1, sec_b2, sec_b3}` | LE32 seconds |
| Timer stop | `01 64 A2` | `{0x00, 0x00, 0x00, 0x00}` | |
| Timer request | `01 65 A2` | — | request current timer |

---

## Vital Commands (ESP→MCU)

Vital commands use flat TLV payloads (same tag/len/value format as status).

| Command | CMD bytes | Payload (TLV) | Notes |
|---------|-----------|---------------|-------|
| Power on/off | `02 00 50` | `{01 01 on/off}` | on=`0x01`, off=`0x00` |
| Fan speed 1–4 | `02 03 55` | `{01 01 speed}` | speed = 1–4 |
| Fan mode Manual | `02 02 55` | `{01 01 00}` | |
| Fan mode Sleep | `02 02 55` | `{01 01 01}` | |
| Fan mode Auto | `02 02 55` | `{01 01 02}` | |
| Fan mode Pet | `02 02 55` | `{01 01 05}` | |
| Auto mode Default | `02 02 55` | `{02 01 00  03 02 00 00}` | tag01=mode, tag03=room_size |
| Auto mode Quiet | `02 02 55` | `{02 01 01  03 02 00 00}` | |
| Auto mode Efficient | `02 02 55` | `{02 01 02  03 02 size_lo size_hi}` | see [Room Size Encoding](#room-size-encoding) |
| Display on | `02 04 55` | `{01 01 64}` | |
| Display off | `02 04 55` | `{01 01 00}` | |
| Child lock on/off | `02 40 51` | `{01 01 01}` / `{01 01 00}` | |
| Light detect on/off | `02 11 55` | `{01 01 01}` / `{01 01 00}` | |
| Timer set | `02 19 50` | `{01 04 b0 b1 b2 b3}` | tag01: LE32 seconds |
| Timer stop | `02 19 50` | `{01 04 00 00 00 00}` | |
| Timer request | `02 1A 50` | — | |
| Wi-Fi LED on | `02 18 50` | `{01 01 01  02 02 7D 00  03 02 7D 00  04 01 00}` | |
| Wi-Fi LED off | `02 18 50` | `{01 01 00  02 02 F4 01  03 02 F4 01  04 01 00}` | |
| Wi-Fi LED blink | `02 18 50` | `{01 01 02  02 02 F4 01  03 02 F4 01  04 01 00}` | |

---

## Sprout Commands (ESP→MCU)

Sprout uses the Vital command set plus these additional commands.

### Nightlight — CMD `02 0B 55`

TLV payload:

| Tag | Len | Value | Description |
|-----|-----|-------|-------------|
| `01` | 1 | `0x01`/`0x00` | on/off |
| `02` | 1 | 0–100 | brightness % |
| `03` | 2 | LE16 Kelvin | color temperature (e.g. `0x07D0`=2000K, `0x0DAC`=3500K) |

### Breathing — CMD `02 0C 55`

TLV payload (tags sent out of numeric order — order matters for MCU):

| Tag | Len | Value | Description |
|-----|-----|-------|-------------|
| `01` | 1 | `0x01` | breathing on |
| `03` | 2 | LE16 Kelvin | color temperature |
| `02` | 1 | 1–10 | cycle time in seconds |
| `04` | 1 | 0–100 | min brightness % |
| `05` | 1 | 0–100 | max brightness % |

### White Noise — CMD `02 07 55`

TLV payload:

| Tag | Len | Value | Description |
|-----|-----|-------|-------------|
| `01` | 3 | `{active, volume, sound_index}` | active: 0/1; volume: 0–255; sound_index: 0–14 |
| `03` | 3 | `{0x05, 0x08, 0xF2}` | constant trailer — purpose unknown |

### White Noise Fan Mode — CMD `02 02 55`

Payload: `{0x10, 0x01, 0x01}` (enable) / `{0x10, 0x01, 0x00}` (disable)

### AQI Scale — CMD `02 06 55`

TLV: `{01 02 aqi_lo aqi_hi}` — LE16 AQI max value (e.g. `0x01F4`=500)

### Async Events — CMD `02 08 55` (MCU→ESP)

TLV:

| Tag | Len | Value | Description |
|-----|-----|-------|-------------|
| `01` | 1 | `0x01`/`0x00` | button press / release |
| `03` | 3 | `{active, volume, sound_index}` | white noise state change |
| `04` | 1 | `0x01`/`0x00` | cover open / closed |

---

## Room Size Encoding

Room sizes are transmitted as raw integer values derived from square feet.

| Series | Formula | Direction |
|--------|---------|-----------|
| Vital (tag `0x10`) | `raw = round(m² × 10.764 × 1.3)` | ESP→MCU |
| Core / Core600S | `raw = round(m² × 10.764 × 3.15)` | ESP→MCU |

Decode (MCU→ESP): `m² = raw / (10.764 × factor)` where factor = 1.3 (Vital) or 3.15 (Core).

---

## Timer Encoding

| Series | CMD | Payload |
|--------|-----|---------|
| Core set/stop | `01 64 A2` | LE32 seconds |
| Core request | `01 65 A2` | empty |
| Vital set/stop | `02 19 50` | TLV `{01 04 sec_le32}` |
| Vital request | `02 1A 50` | empty |

Timer status pushed from MCU contains remaining seconds and initial seconds (Core sends both as two LE32 values; Core200S sends only remaining).
