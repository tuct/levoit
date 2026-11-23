[← Back](../../README.md)

# Levoit Eleva 300X - Custom Firmware (ESPHome)

> **Work in progress** — research and UART capture phase. No firmware yet.

## Quick Facts

| Item | Value |
|------|-------|
| Model | Eleva 300X (EU: LAP-C341S-WEU, US: LAP-C341S-WUSZ) |
| Released | August 2026 |
| Tested MCU FW | 1.0.06 as shipped (WiFi FW 1.0.07, VeSync SDK v1.3.1, ESP-IDF dc489539); updated via VeSync to WiFi FW 1.0.08 + new MCU FW on 2026-11-20 |
| Internal name | Salute 300 (EU, Amazon), label 1321-B2516B-C |
| ESP Module | ESP32-C3 (QFN32, rev v0.4), 4 MB embedded flash (XMC), single core |
| Board | PCB A: main board (MCU + ESP) / PCB B: HT-PCB-1321-B2516A-C-V04 (display) |
| PM Sensor | Cubic PM2008MZ (laser, PM1.0 / PM2.5 / PM10) |
| Fan Speeds | 4 (1–3 + Turbo) |
| Modes | Manual, Auto |
| CADR (spec) | 161 CFM smoke / dust / PM2.5, 172 CFM pollen (~273 m³/h) |
| Noise (spec) | 20–50 dB |
| Room Size | up to 250 ft² (~23 m²) at 4.8 ACH |
| Power | up to 25.5 W, 100–240 V 50/60 Hz |
| Dimensions | 23.8 × 23.8 × 38.3 cm (9.4 × 9.4 × 15.1 in), 4.5 kg (9.9 lb) |
| ESPHome | TBD |

## Stock Features (from manufacturer / reviews)

Firmware support for these is still to be decided once the UART protocol is known.

| Feature | Notes |
|---------|-------|
| Fan | 4 speeds: 1–3 + Turbo (reviews also mention a Sleep mode) |
| Auto Mode | Driven by the PM sensor, configurable in the app |
| Dynamic Manual | Manual speed that jumps to Turbo when pollution is detected, then returns |
| Air quality display | Onboard screen with PM1, PM2.5, PM10 and an AQI score; LED ring color by air quality |
| Display off | Works at any fan speed, including Turbo |
| Screen lock | Touch panel lock (likely the child lock) |
| Filters | HEPA main filter (~24 months) + separate replaceable carbon filter (6–12 months depending on type) |
| Filter tracking | Separate life tracking / reminders for main and carbon filter (stock: VeSync app) |

Carbon filter options: Essential (90 g, ~6 months), Pet (200 g), Smoke & Cooking (240 g), Premium (260 g, ~12 months).

## Measured Values (HouseFresh)

| Speed | Noise @ 3 ft | Power |
|-------|-------------|-------|
| Standby | – | 0.65 W |
| Sleep | 35.8 dBA | 1.85 W |
| 1 | 36.8 dBA | 5.33 W |
| 2 | 39.8 dBA | 6.91 W |
| 3 | 44.9 dBA | 10.03 W |
| Turbo | 55.4 dBA | 23.91 W |

Measured CADR: 165 CFM (PM1) at Turbo, 115 CFM (PM1) below 45 dBA.

## Hardware

### PCB A — main board

![PCB A](images/pcb_a.png)

| Ref | Part / Pins (from silkscreen) | Notes |
|-----|------------------------------|-------|
| WIFI1 | Espressif ESP32-C3-SOLO-1 module (FCC ID 2AC7Z-ESPC3SOLO) | Same module as the Vital 100S / 200S; MCU UART on module pins 27 / 28 = **IO18 (RX) / IO19 (TX)** |
| J3 | EN, GND, +3V3, TXD0, RXD0, IO0 | ESP programming / UART header |
| MCU1 | 44-pin QFP, part TBD | Main MCU, talks to the ESP over UART |
| J5 | +5V, DIO, GND, CLK | MCU debug header (SWD-style) |
| U4 | 28-pin SSOP, part TBD | |
| Dust connector | +5V_DUST, DUST_… | PM2008MZ sensor cable |
| J8 | GND, VOC_SCL(TX), VOC_SDA(RX), +5V_VOC | VOC sensor connector (VOC传感器), appears unpopulated |
| J6 | W_DAT, GND, +3V3, W_CLK | Temp / humidity sensor connector (温湿度传感器), appears unpopulated |
| J9 | +5V, RGBR, RGBG, RGBB | RGB output |
| Fan connector | M_24V … M_FG | 24 V fan motor with speed feedback (FG) |

### ESP32-C3 stock flash layout

Read with `esptool read-flash 0 ALL`. Secure Boot and flash encryption are **disabled**, so the stock firmware can be backed up and restored.

| Name | Type | Offset | Size |
|------|------|--------|------|
| nvs | data/nvs | 0x009000 | 16 KiB |
| otadata | data/ota | 0x00d000 | 8 KiB |
| phy_init | data/phy | 0x00f000 | 4 KiB |
| ota_0 | app | 0x010000 | 1536 KiB |
| ota_1 | app | 0x190000 | 1536 KiB |
| usercfg | data/nvs | 0x310000 | 128 KiB |
| fac_data | data/nvs | 0x330000 | 64 KiB (factory device keys) |
| storage | data/0x0f | 0x340000 | 128 KiB |
| historydata | data/nvs | 0x360000 | 128 KiB |
| log | data/nvs | 0x380000 | 64 KiB |

### ESP ↔ MCU UART pins

115200 8N1 on module pins 27 / 28 of the **ESP32-C3-SOLO-1** (FCC ID 2AC7Z-ESPC3SOLO), measured with a logic analyzer on the pads.

| Signal | Module pin | ESP32-C3-SOLO-1 pin name | ESPHome pin | Saleae |
|--------|-----------|---------------------------|-------------|--------|
| MCU TX → ESP RX | 27 | IO18 | `rx_pin: GPIO18` | ch 0 |
| ESP TX → MCU RX | 28 | IO19 | `tx_pin: GPIO19` | ch 1 |

Same module and pins as the [Vital 100S](../levoit-vital100s) and [Vital 200S](../levoit-vital200s), both tested on the stock module. Flash: DIO at 80 MHz, 4 MB, as the stock firmware's image header (bootloader and app, both dumps) sets it.

Don't go by the classic **ESP32-SOLO-1** datasheet: it has the same footprint, but there pins 27 / 28 are IO16 / IO17. On the C3, GPIO16 / GPIO17 are internal flash pins, which ESPHome refuses. GPIO18 / GPIO19 are the C3's USB pins, so ESPHome warns that USB-Serial-JTAG uses them; that is expected, the log runs on UART0.

The stock app agrees: `vesync_uart_init` passes `tx = 19, rx = 18` (and 115200) to `uart_set_pin()` in both dumped builds (May 2026 in `ota_0`, Wi-Fi 1.0.08 from Jul 2026 in `ota_1`).

The stock firmware updates the MCU in pass-through mode: the image is streamed from the cloud over UART to the MCU and is not stored on the ESP flash first.

The VeSync update downloads from `fw.vesync.com` by hostname. If the device's DNS lookups fail (`getaddrinfo() error=202` in the UART log), the update aborts at about 4% in the app. Adding 8.8.8.8 as a DNS server fixed it here.

### PCB B — display board (HT-PCB-1321-B2516A-C-V04)

![PCB B](images/pcb_b.png)

- 3-digit 7-segment display plus 4 + 5 indicator LEDs under the light guide
- 20 RGB LEDs (RGB1–RGB20) forming the air-quality ring
- 5 spring touch keys (KEY2–KEY6 visible), each with a backlight LED
- Buzzer (BUZ1)
- OP1: small sensor near the top, possibly an ambient light sensor (TBD)
- Label: *Salute 300, EU, Amazon, MCU1: 1.0.06, WIFI1: 1.0.07, 1321-B2516B-C*

## UART Protocol (WIP)

ESP ↔ MCU link, captured with a Saleae Logic 8 (on the capture: channel 0 = MCU TX, channel 1 = ESP TX). **115200 8N1.**

Same framing as the other Levoit devices: `A5 | type | seq | len (u16 LE) | checksum | 02 <cmd lo> 55 00 | flat TLV…`. Type `22` is a command, `12` an acknowledgement and `02` a report that needs no reply. Checksum = `0xFF - (sum of all bytes except [5])`, like `levoit_checksum()`.

Periodic traffic (about once per second):

| Dir | Type / Cmd | Content |
|-----|-----------|---------|
| MCU → ESP | RPT `5523` | `01` PM1, `02` PM2.5, `03` PM10 (µg/m³, display caps at 999), `0b` AQI, `0c` air-quality level (1–4) |
| MCU → ESP | RPT `5500` | full status (see below) |
| ESP → MCU | CMD `5523` | echoes PM/AQI back, `04`–`0a` = 0xFFFF (no temp/humidity/VOC sensors) |
| ESP → MCU | CMD `5521` `0c=0` | MCU ACK carries `01`=1, `07`=79200, `08`=21600, `0b`=1800 (meaning TBD) |

**The Eleva 300X uses the Vital protocol.** The status `5500` keys and the ESP → MCU commands match [vital_status.cpp](../../components/levoit/vital_status.cpp) / [vital_commands.cpp](../../components/levoit/vital_commands.cpp). Panel buttons are handled by the MCU locally and only show up in the status report; app actions send the commands below.

Status `5500` (MCU → ESP, about once per second):

| Key | Meaning (Vital name) | Values seen on the Eleva |
|-----|---------|--------|
| `01` | MCU version (patch, minor, major) | `0d 00 01` = 1.0.13 (after the 2026-11-20 update) |
| `02` | power | 1 |
| `03` | fan mode | 0 manual, 1 sleep, 2 auto |
| `04` | fan level | 1–3, 4 = turbo, 0 in sleep |
| `05` | fan speed setting | 1–4 (kept while in auto/sleep) |
| `06` | display illuminated | 1 on, 0 off (sleep turns it off after ~3 s) |
| `09` | air-quality level | 1–4 |
| `0a` | air-quality detail | counts up slowly on the Eleva, meaning TBD |
| `0b` / `0c` / `0d` | PM2.5 / PM1 / PM10 | µg/m³ |
| `0e` | child lock | 0/1 |
| `13` | light detection | 0/1 |
| `2c` | AQI | Eleva-specific |
| `17` | dark detected | 0/1 |
| `34` | ambient light sensor, raw | ~4400 in a lit room, ~700 covered |
| `2a` `2b` `2e`–`31` | missing sensors | 0xFFFF |

Commands (ESP → MCU, from the VeSync app, each ACKed by the MCU within ~40 ms):

| Action | Command + payload | Status key | Existing `CommandType` |
|--------|-------------------|------------|------------------------|
| Power on / off | `02 00 50` + `01 01 01` / `00` | `02` (off: `04` = 255, `06` = 0) | `setDeviceON` / `OFF` |
| Fan level 1–3 / turbo | `02 03 55` + `01 01 0N` (N = 1–4) | `04`, `05` | `setDeviceFanLvl1`–`4` (turbo is level 4, not mode 4 as on the EverestAir) |
| Manual / sleep / auto | `02 02 55` + `01 01 00` / `01` / `02` | `03` | `setFanModeManual` / `Sleep` / `Auto` |
| Display on / off | `02 04 55` + `01 01 64` / `01 01 00` | `06`, `12` | `setDisplayOn` / `Off` (not possible from the app in sleep mode) |
| AQI ring bright / dim | `02 04 55` + `02 01 00` / `02 01 01` | `32` (0 bright, 1 dim) | **new** |
| Child lock on / off | `02 40 51` + `01 01 01` / `00` | `0e` | `setDisplayLockOn` / `Off` |
| Light detection on / off | `02 11 55` + `01 01 01` / `00` | `13` (`17` = dark detected) | `setLightDetectOn` / `Off` |
| Mute on / off | `02 17 55` + `01 04 01 00 00 00` / `01 04 00 00 00 00` (4-byte value) | `2d` (1 muted) | **new** |

Not captured yet: filter reset.

### Auto-mode configuration `5521`

The quiet period and adaptive start are stored and run **on the MCU**. Auto mode itself also runs on the MCU, which adjusts `04` from its own PM reading. The ESP reads the block about once per second with CMD `5521` `0c 01 00`; the MCU answers in its ACK with the full block. To change settings the ESP sends a CMD `5521` with only the changed keys; the MCU ACKs and also pushes the new block as a REPORT.

| Key | Meaning | Values seen |
|-----|---------|-------------|
| `01` | TBD | 1, 2, 3 |
| `02` | adaptive start, all modes (powers on/off by air quality; VeSync describes it as cloud-only, so the MCU may only store it) | 0/1 |
| `03` | dynamic manual (manual speed jumps to turbo on pollution), Manual mode only | 0/1 |
| `04` | dynamic sleep, Sleep mode only | 0/1 |
| `05` | quiet period on/off, all modes (lowers the fan by one level while active) | 0/1 |
| `06` | quiet period trigger: **0 = light sensor (darkness)**, 1 = time window `07`/`08` | 0/1 |
| `07` | quiet window start, seconds after midnight (u32) | 79200 = 22:00, 81000 = 22:30 |
| `08` | quiet window end, seconds after midnight (u32) | 21600 = 06:00, 25200 = 07:00 |
| `0b` | TBD (u16) | 1800 |
| `09` | event, sent alone by the MCU: **7 = quiet period started, 8 = ended** | 7, 8 |

The ESP always writes keys `01`–`04` as one group and `05`–`08` as one group. Set examples: `02 21 55` + `05 01 01 06 01 01 07 04 60 35 01 00 08 04 60 54 00 00` (quiet on, timer, 22:00–06:00), `02 21 55` + `01 01 02 02 01 01 03 01 00 04 01 01` (adaptive start and dynamic sleep on, key 01 echoed).

Verified with the sensor covered (capture `quiet_light3`): quiet on with trigger light, sensor covered → status `34` falls from ~4400 to ~700, `17` dark = 1, `5521 09=7`, fan level 4 → 3, display off; uncovered → `09=8`, fan back to 4. With trigger = timer, the MCU needs the local time from the ESP.

### Local time `5522`

At boot the MCU sends CMD `02 22 55` with no payload (twice, ~1 s apart). The ESP ACKs it and, once it has network time, sends CMD `02 22 55` + `01 04 <epoch LE32>`, where the epoch is **local** time (UTC + zone offset): `1795260449` was sent at 11:27:29 CET.

Other boot traffic from the stock ESP: Wi-Fi LED `02 18 50` (blinking, then on), `02 24 55` (same payload as the Wi-Fi LED off, TBD), filter `02 05 55` (below), status request `02 01 55` (empty; full status comes back in the ACK).

### Filter `5505`

Filter life is kept by the ESP / cloud, not the MCU: the status report has no filter percentage, and "done replace" for the main or the carbon filter in the app sends nothing to the MCU. The app has a genuine-filter check; a genuine filter cannot be reset manually, only after unlinking it.

| Action | Frame | Direction |
|--------|-------|-----------|
| Filter reset on the panel | `02 05 55` + `03 01 00` | **MCU → ESP** (SEND, ESP ACKs) |
| Unlink + reset in the app | `02 05 55` + `03 00` (key 03, empty value), then `04 01 00` | ESP → MCU |
| Link a genuine filter in the app | `01 01 00`, `04 01 01`, `01 01 00` | ESP → MCU |
| Every boot | `04 01 <genuine 0/1>`, then `01 01 00` | ESP → MCU |

Key `04` = genuine filter linked (no visible effect on the unit either way), `03` = reset, `01` always 0 (TBD). Captures: `filter_reset`, `filter_reset2`, `filter_nongenuine_boot`, `filter_genuine_boot`.

### Timer

Standard Vital timer: set `02 19 50` + `01 04 <seconds> 02 01 00` (0 = cancel); query `02 1A 50` (ACK carries `01` total, `02` remaining); while running, the MCU pushes `02 1B 50` every second **as a REPORT** with `02` remaining, `01` total. Seen: 1800, 3600, 28800 s.

The Saleae Logic 2 captures behind this section are in [uart/saleae/](uart/saleae/), with an index of what each one shows.

## ESPHome (WIP, not tested on hardware)

`model: ELEVA300X` in the levoit component. Configs: [levoit-eleva-300x.yaml](levoit-eleva-300x.yaml) (GitHub source) and [levoit-eleva-300x_dev.yaml](levoit-eleva-300x_dev.yaml) (local component), both for the stock ESP32-C3 module; the logger stays on J3 (UART0). For the ESPHome Device Builder: [levoit-eleva-300x-builder.yaml](levoit-eleva-300x-builder.yaml), everything inlined (see [devices/README.md](../README.md#esphome-web-builder--dashboard-no-local-setup)). For a replacement Seeed XIAO ESP32-S3 instead of the stock module (RX GPIO5 = pad D4 to module pin 27, TX GPIO6 = pad D5 to pin 28, stock EN held low): [levoit-eleva-300x-s3.yaml](levoit-eleva-300x-s3.yaml), [_dev](levoit-eleva-300x-s3_dev.yaml) and [builder-s3](levoit-eleva-300x-builder-s3.yaml); untested. Or a Seeed XIAO ESP32-C3 (RX GPIO5 = pad D3, TX GPIO6 = pad D4, same module pins 27 / 28, the same pins as the Vital 100S / 200S XIAO configs): [levoit-eleva-300x-c3.yaml](levoit-eleva-300x-c3.yaml), [_dev](levoit-eleva-300x-c3_dev.yaml), [builder-c3](levoit-eleva-300x-builder-c3.yaml); untested.

Before flashing:

1. Back up the stock flash (`esptool --chip esp32c3 read-flash 0 ALL backup.bin`, IO0 held low during reset).
2. Flash over J3 (TXD0 / RXD0 / IO0). The configs already use the MCU pins `rx_pin: GPIO18` / `tx_pin: GPIO19` (see [ESP ↔ MCU UART pins](#esp--mcu-uart-pins)).

Implemented: everything in the tables above. Filter reset: the `reset_filter_stats` button sends `03 00` + `04 01 00` like the stock app; a panel reset resets the ESP-side filter estimate. At boot ESPHome sends `04 01 00` (not genuine), like a stock boot with an unlinked filter; it cannot verify a filter, and the flag has no visible effect. The quiet period with trigger = timer needs `time_id` on the `levoit:` block (the configs use Home Assistant time) so the MCU gets the local time.

## Notes

- UART captures go in [uart/](uart/)
- Teardown / PCB photos go in [images/](images/)

## Sources

- [Levoit press release (TNGlobal reprint), Aug 2026](https://technode.global/?p=991105)
- [HouseFresh review](https://housefresh.com/levoit-eleva-300x-review/)
- [RTINGS review](https://www.rtings.com/air-purifier/reviews/levoit/eleva-300x)
