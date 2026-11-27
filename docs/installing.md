[← Back](../README.md)

# Installing ESPHome on a purifier

The steps every device guide shares, written once. Each guide under
[`devices/`](../devices/) follows the same order and adds only what is
particular to its model: the screws, the pads, the board revisions, the
photos. Read the guide for your model first, and come here for the detail of
a step.

1. [Build the firmware](#1-build-the-firmware): with Home Assistant
   (recommended) or by hand
2. [Disassembly](#2-disassembly): open the unit and find the board
3. [Flash the original ESP32](#3-flash-the-original-esp32), **or**
4. [Add your own ESP32](#4-add-your-own-esp32) and leave the original in place
5. [Assembly](#5-assembly): close it up and check it comes online

[Back to stock](#back-to-stock) is at the end.

The hardware-hack devices (LV-PUR131, LV-PUR131S, Mini) replace or modify the
board instead of steps 3 and 4; their guides describe that part on their own.

## Choosing a route

| | **Add your own ESP32** (recommended) | **Flash the original ESP32** |
|---|---|---|
| What | A small ESP32 of your own, wired to the MCU's UART; the original ESP stays on the board, held in reset | The purifier's own ESP32, flashed with ESPHome |
| Parts | A [Seeed XIAO ESP32-C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html?sensecap_affiliate=3gGMlPv&referring_service=link) or [XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?sensecap_affiliate=3gGMlPv&referring_service=link) (or any ESP32 ESPHome supports) and four or five wires | A USB-UART adapter and wires to the board's pads |
| First flash | Over USB at your desk, before it goes in | Over the board's UART pads, with the purifier open |
| A bad build | Only hits your ESP; reflash it over USB | Can leave the purifier offline until you open it again |
| Back to stock | Remove the wires; the stock firmware was never touched | Restore your backup over the pads |
| Locked modules | Works on every model | Only modules without secure boot (not the Philips AC0650) |

A third way, keeping both ESPs on the UART with a two-position switch on the
original's `EN` to choose which one runs, keeps the stock firmware one flick
away for MCU updates. Switch only while the purifier is unplugged.

## 1. Build the firmware

Every device folder has a YAML file per board, and a builder file of each with
everything inlined:

| File | Board | Use it with |
|------|-------|-------------|
| `<model>.yaml` | the original ESP32 | ESPHome CLI, from a clone of this repo |
| `<model>-c3.yaml` | Seeed XIAO ESP32-C3 | ESPHome CLI |
| `<model>-s3.yaml` | Seeed XIAO ESP32-S3 | ESPHome CLI |
| `<model>-builder*.yaml` | the same three | ESPHome Device Builder or web builder: one file, no clone, no `!include` |

The builder files pull the component from the **`stable`** branch, which moves
to every release. The device guide lists the files its folder has.

### With Home Assistant (recommended)

The [Levoit ESPHome](https://github.com/tuct/levoit-esphome-ha) integration
makes the config, builds it, and keeps the device updated afterwards. It needs
the **ESPHome Device Builder add-on** (Home Assistant OS or Supervised).

1. Install the integration:
   [setup steps](https://github.com/tuct/levoit-esphome-ha#installation).
2. **Settings → Devices & services → Levoit ESPHome → Add device**, or *Add
   device* on its sidebar page.
3. Pick **New device**, the model, and the ESP: original, Seeed XIAO
   ESP32-C3, ESP32-C3 or Seeed XIAO ESP32-S3. For any other board pick
   **Other ESP** and give the ESPHome board name and the RX / TX pins.
4. It writes the config into the Device Builder, with a new API encryption
   key and your Wi-Fi from the Device Builder's `secrets.yaml`, and compiles it.
5. Flash it once over USB from the Device Builder: the config → **Install** →
   **Plug into this computer**. For your own ESP32 that is its USB port; for
   the original ESP32 it is a USB-UART adapter on the board's pads
   ([step 3](#3-flash-the-original-esp32)).
6. Once the ESP is on your Wi-Fi, the integration adds it to Home Assistant by
   itself. New releases then show up under **Settings → Updates** and install
   over the air.

Which model has which template:
[supported models](https://github.com/tuct/levoit-esphome-ha#supported-models).
A model without one (the humidifiers, the hardware-hack devices) is built by
hand.

### Manually

**ESPHome Device Builder or web builder, no clone.** Take the builder file for
your board from the device folder, paste it into the ESPHome Device Builder
(or upload it to the [ESPHome web builder](https://builder.esphome.io)), and
add `wifi_ssid`, `wifi_password`, `wifi_ap_password`, `api_key` and
`ota_password` to its `secrets.yaml`. For every merge instead of every release,
change `ref: stable` to `ref: main` in your copy.

**ESPHome CLI, from a clone.**

1. In the device folder, copy `secrets-example.yaml` → `secrets.yaml` and fill
   in your Wi-Fi and encryption key. Keep `secrets.yaml` out of commits.
2. Change the device name in the YAML if you run more than one unit.
3. Compile:

   ```bash
   esphome compile levoit-core300s.yaml
   ```

4. Flash over USB (your own ESP32) or the UART pads (the original), naming the
   port, or leave `--device` out to be asked:

   ```bash
   esphome run levoit-core300s.yaml --device COM5
   ```

The minimum ESPHome version is in the
[component README](../components/levoit/README.md). The UART pins per board
are there too, should you build your own config.

## 2. Disassembly

Unplug the purifier first. The device guide has the screws, clips and photos.

**Look at the board before you open anything.** Levoit's FCC filings are under
grantee [`2ARBY`](https://fccid.io/2ARBY) (Arovast Corporation) and most
include internal photos of the bare PCB: enough to spot the Wi-Fi module and
the test pads. Examples:
[Core 200S](https://fccid.io/2ARBY-CORE-200S) ·
[Core 300S](https://fccid.io/2ARBY-CORE-300S) ·
[Core 600S](https://fccid.io/2ARBY-CORE600S) ·
[Sprout](https://fccid.io/2ARBY-B381S). The hyphenation is inconsistent
(`CORE-300S` but `CORE600S`), so browse the grantee index rather than guessing
an ID.

Photograph each connector before you unplug it.

## 3. Flash the original ESP32

### Connect

Solder or clip wires to the original ESP32's **TX, RX, IO0, 3V3 and GND**
(the device guide shows where; on some boards they are through-holes or a
header) and connect them to a 3.3 V USB-UART adapter: adapter TX → ESP RX,
adapter RX → ESP TX.

Hold **IO0 to GND while it powers up** to start the bootloader. Power it from
the adapter's 3V3, with the purifier unplugged from the mains.

### Back up the stock firmware

Do this before anything else. It is the only way back to stock on this route.

```bash
esptool read_flash 0 ALL levoit-core300s-backup.bin
```

Keep the file. A backup can fail on some boards; then go on only if you accept
that there is no way back.

### Flash

With IO0 held low again at power-up, flash the config from
[step 1](#1-build-the-firmware): **Install → Plug into this computer** in the
Device Builder, or

```bash
esphome run levoit-core300s.yaml --device COM5
```

Remove the IO0 wire, power-cycle, and watch the log for the ESP joining your
Wi-Fi and the MCU's status frames arriving. From now on updates go over the
air.

## 4. Add your own ESP32

### Flash it first

Flash the config from [step 1](#1-build-the-firmware) over USB at your desk,
and check it joins your Wi-Fi, before it goes into the purifier.

### Wire it in

| Connection | Notes |
|-----------|-------|
| `5V` or `3V3`, and `GND` | Tap the board's own supply; the device guide says where |
| ESP `RX` ← MCU `TX` | The line carrying the status frames |
| ESP `TX` → MCU `RX` | The line the stock module transmitted on |
| Original ESP32 `EN` → `GND` | Parks the original, see below |

**Park the original ESP32.** Pull its `EN` (enable) pin to GND: it stays
soldered in place, held in reset, and the change is reversible. Without this,
both ESPs drive the same line. Check for an existing pull-up before choosing a
resistor: against a 10k pull-up, a 10k pull-down only reaches half the rail
and may not hold reset. More in
[adding a new device](./adding-a-new-device.md#4-park-the-original-wi-fi-module).

**Watch the logic level and the strapping pins.** Measure before connecting a
GPIO directly. On an ESP32-C3, `GPIO8`/`GPIO9` are strapping pins and `GPIO9`
is usually the BOOT button, so a line held low there at reset drops the board
into download mode instead of running your firmware.

Insulate the board and every joint.

## 5. Assembly

Reverse the disassembly; the device guide notes what to watch for on each
model. In general:

- Route the new wires away from the fan and from screw posts, so nothing is
  pinched when the halves close.
- Fix your ESP32 so it cannot rattle against the board, with tape or a dab of
  hot glue on its insulated side.
- Plug every connector back where your photos show it.
- Do not overtighten screws into plastic.

Plug it in. The ESP joins your Wi-Fi and shows up in Home Assistant, and the
log shows the MCU's status frames. If it connects but no status frames
arrive, check first that RX and TX are not swapped and that the original
ESP32 is parked.

## Back to stock

- **Your own ESP32:** unplug, remove its wires and the `EN` wire. The original
  ESP32 runs the stock firmware again.
- **Original ESP32:** connect as in [step 3](#connect), with IO0 low, and write
  the backup back:

  ```bash
  esptool erase_flash
  esptool write_flash 0x00 levoit-core300s-backup.bin
  ```
