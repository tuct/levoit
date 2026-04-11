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

1. Capture UART traffic at **9600 baud, 8N1** — connect a logic analyzer to both the **ESP TX** and **MCU TX** lines with shared GND
2. Add an **Async Serial** analyzer on the **ESP TX** channel (9600 baud, 8N1)
3. Add a second **Async Serial** analyzer on the **MCU TX** channel
4. Add a **Levoit UART Extractor** HLA on top of the first Async Serial, set **Channel** to `ESP->MCU`
5. Add a second **Levoit UART Extractor** HLA on top of the second Async Serial, set **Channel** to `MCU->ESP`

Both HLAs run side by side so you can see the full request/response exchange in one view.
