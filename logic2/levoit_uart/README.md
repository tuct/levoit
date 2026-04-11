# Levoit UART Extractor — Saleae Logic 2 HLA

A High-Level Analyzer (HLA) for Saleae Logic 2 that decodes the Levoit flat-TLV UART protocol used by Core 300S / 400S / 600S and Vital 100S / 200S air purifiers.

## Protocol

Packets start with `0xA5`, followed by a message type byte, command bytes, a length byte, and a payload:

```
A5  <type>  <cmd0> <cmd1> <cmd2>  <len>  [payload...]  <checksum>
```

| Type byte | Label |
|-----------|-------|
| `0x22`    | SEND (ESP→MCU) |
| `0x12`    | ACKN (MCU→ESP, Core/Vital) |
| `0x52`    | RESP (MCU→ESP, Core 600S) |

Decoded frames are displayed as:

```
[ESP->MCU] SEND(0x22) |  CMD=01 40 B0  |  PAY=01
```

## Installation

1. Open **Logic 2**
2. Go to **Extensions** (puzzle piece icon) → **Load Existing Extension**
3. Select the `levoit_uart` folder (the one containing `extension.json`)

## Settings

| Setting | Options | Description |
|---------|---------|-------------|
| Channel | ESP->MCU / MCU->ESP / - | Labels each frame with the traffic direction |
| Include Raw Bytes | No / Yes | Appends the full raw hex bytes to each frame |

## Usage

1. Locate the correct solder points on the PCB — see the individual device README for the exact pads
   > **Note:** The debug pin header RX/TX pins are used to flash the ESP32. They are **not** the UART line between the ESP32 and the MCU. Tap into the dedicated ESP↔MCU communication pads (test points or vias near the ESP32), not the header.
2. Capture UART traffic at **9600 baud, 8N1** — connect a logic analyzer to both the **ESP TX** and **MCU TX** lines with shared GND
2. Add an **Async Serial** analyzer on the **ESP TX** channel (9600 baud, 8N1)
3. Add a second **Async Serial** analyzer on the **MCU TX** channel
4. Add a **Levoit UART Extractor** HLA on top of the first Async Serial, set **Channel** to `ESP->MCU`
5. Add a second **Levoit UART Extractor** HLA on top of the second Async Serial, set **Channel** to `MCU->ESP`

Both HLAs run side by side so you can see the full request/response exchange in one view.

## What to Capture

Capture separate, clearly labelled dumps for each action — one action per capture makes it much easier to identify which bytes correspond to which command:

| Dump | Action |
|------|--------|
| `bootup` | Power on → wait until Wi-Fi connected and app shows online |
| `speed_1-4_app` | Switch through fan speeds 1 → 2 → 3 → 4 via the **app** |
| `speed_1-4_device` | Switch through fan speeds 1 → 2 → 3 → 4 via the **physical buttons** |
| `mode_app` | Switch through all modes (Manual / Auto / Sleep / Pet) via the **app** |
| `mode_device` | Switch through all modes via the **physical buttons** |
| `auto_mode` | Switch through all auto mode sub-options (Default / Quiet / Room Size / etc.) |
| `display_on_off` | Toggle the display on and off |
| `child_lock` | Enable and disable child lock |
| `timer` | Set a timer via the app |
| `filter_reset` | Reset filter stats |
| *(model-specific)* | Any unique features: lights, white noise, CO₂ sensor, etc. |

Label each file clearly (e.g. `core300s_2.0.11_speed_app.txt`).

Use **Export Data** in Logic 2 to export the analyzer results as a text/CSV file. A plain text file with the decoded HLA output is all that's needed — no `.sal` session files.

> **Example:** See [`devices/levoit-sprout/uart/uart_dumps`](../../devices/levoit-sprout/uart/uart_dumps) for a real Sprout dump covering boot, speed switching, light modes, and white noise — each section labelled with the action performed.
