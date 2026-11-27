# LEH-S602S-WUS stock firmware notes

Read-only analysis of a 4 MiB ESP32-C3 flash backup supplied by jasongill.
SHA-256: `df81c80f3edb0fc86f6e667bebe2af2b319783c79f1ea654c51ccbf18d3678ef`.
Both app images have valid ESP checksums and validation hashes. Their app-version
fields are empty; both contain `LEH-S602S-WUS` and ESP-IDF `dc489539`.

| Slot | Flash offset | Build time |
|---|---|---|
| ota_0 | `0x10000` | 2025-08-26 14:01:01 |
| ota_1 | `0x190000` | 2026-05-12 10:26:29 |

These are Wi-Fi-module builds, not different MCU versions. The dates do not
identify the active slot. The full backup includes provisioning partitions
and is not included here.

## Status fields

Addresses below are in ota_1. Its IROM segment maps flash `0x1c0020` to
`0x42000020`; DROM maps flash `0x190020` to `0x3c0f0020`. Function names
come from diagnostic strings referenced by the code, not ELF symbols.

- `humidifier_update_dev_status` (`0x4200719a`) extracts one byte from tag
  `02` at `0x42007256–0x42007260`, then logs it as `enabled` at
  `0x42007348–0x42007360`. Tag `00` participates in version/registration
  handling; its exact meaning was not established. This supports the
  [power captures](uart/power.txt), but not other models/MCU versions.
- `humidifier_update_dev_private_status` extracts tag `14` at
  `0x4200993e–0x42009948`. The unpack helper (`0x4200de32`) zero-fills
  the destination and copies the smaller of its capacity and the TLV length.
  Its capacity argument of 6 does not mean a six-byte wire field.

| Tag 14 value bytes | Interpretation | Load instruction address |
|---|---|---|
| 0–1 | Remaining seconds, little-endian uint16 | `0x420099a4` (`lhu`) |
| 2 | Auto-dry on/off | `0x42009950` (`lbu`) |
| 3 | Drying active flag | `0x4200996c` (`lbu`) |
| 4 | Drying level | `0x42009988` (`lbu`) |

The older image also uses a 16-bit remaining-time load (`0x42008e22`).
In [drying.txt](uart/drying.txt), section 08, `1F 1C 01 01 01` means
7,199 seconds plus the flags, not 72,735 seconds. The PR separates the
auto-dry flag from time and reports it through the existing switch.

## Command builders and untested leads

Commands use the framing described in [LEVOIT_UART.md](../../LEVOIT_UART.md).
The pack helper is `0x4200dca6`; the send helper is `0x4200767e`.

| Builder | Address | Command and payload |
|---|---|---|
| `humidifier_uart_set_power` | `0x42007878` | `02 00 50`, `01 01 <value>` |
| `humidifier_uart_force_set_power` | `0x420078e8` | `02 00 50`, `02 01 <value>` |
| `humidifier_set_uart_mode` | `0x42007af2` | `02 32 55`, tag 01 mode, optional one-byte tags 02 and 03 |
| `humidifier_uart_set_dry_lvl` | `0x42009c9e` | `02 46 55`, `02 01 <level>` |

A stock scheduling path at `0x42005fce–0x42005fe2` calls the mode builder
with Manual (1) and a level pointer: payload `01 01 01 02 01 <level>`.
That is not a capture of the app leaving Humidity/Dry mode. The PR retains
the separately captured mode-before-speed sequence in
[mode_speed.txt](uart/mode_speed.txt), section 06.

The force-power command might bypass drying; it has not been exercised and
is not used by this PR. Stock-app Pet availability was not established.
Additional leads include completed/interrupted drying events, filter wet-time
tracking, and a humidity-compensation setter in the newer image. A wet-time
path compares against 259,199, consistent with a three-day threshold, but
units and state semantics need further tracing before drawing conclusions.

## Reproducing

Parse the ESP partition table and application segment headers, then inspect
application images with `esptool image-info`. Disassemble an IROM segment with
`riscv32-esp-elf-objdump -D -b binary -m riscv:rv32 --adjust-vma=0x42000020`.
The virtual addresses above allow checking the loads, string references and
command-building calls without relying on guessed function boundaries.
