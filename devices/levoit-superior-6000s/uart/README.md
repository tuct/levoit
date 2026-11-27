# Superior 6000S UART captures

Captured by jasongill on 2026-11-27: stock ESP32-C3 module, MCU **1.0.9**,
RX GPIO18 / TX GPIO19, 115200 8N1. Firmware: ESPHome **2026.10.2**,
customized component based on `72c0201`, with the control fixes ported in
#176. The rebased PR firmware was not flashed for this capture.

ESPHome UART debugging recorded both directions over the native API:
`direction: BOTH`, `dummy_receiver: false`, `after.bytes: 64`,
`after.timeout: 20ms`; logger DEBUG, 1024-byte transmit buffer, serial logging
disabled. UART chunks were joined into complete frames without changing bytes.
Times are UTC receipt times of the first chunk, not wire timings. Repeated
frames and acknowledgements are retained. Every included frame passes the
length and checksum checks in [LEVOIT_UART.md](../../../LEVOIT_UART.md).
The ESP's 10-byte ACKs use byte 4 = FF; it is not a length high byte here.

The humidity automation was paused; commands came from Home Assistant.
No panel, cover or refill tests were performed. States below are MCU reports,
not independent physical observations.

| File | Actions and reported result |
|---|---|
| [power.txt](power.txt) | Off, on at Manual 1, off; tag 00 stays 1, tag 02 changes 0/1/0, tag 07 (humidifying) follows |
| [mode_speed.txt](mode_speed.txt) | Manual 5 → Auto → Manual 1; TX `02 32 55` precedes `02 33 55`; status tags 0B/0C finish at 1/1 |
| [drying.txt](drying.txt) | Enable auto-dry, request off: tag 02 stays 1, tag 07 becomes 0, tag 14 byte 3 becomes 1; disable auto-dry and request off: power 0, dry flag 2 |

The mode capture demonstrates the corrected ordering; it does not reproduce
the old speed-before-mode failure. Tag 14 is a five-byte composite value;
its complete bytes must remain available to the decoder.

## Dry controls follow-up

[dry_controls.txt](dry_controls.txt) uses LEH-S602S-WUS / MCU **1.0.9**,
ESPHome 2026.10.2, PR `762aac8` (section 01), then the review fixes (03–07).
Same UART pins and framing; DEBUG logger, default 150-byte chunk limit,
10 ms timeout. Commands came from HA with both auto-dry switches off.

`02 46 55 / 02 01 02` starts High drying from Manual: tag 07 becomes 0,
tag 14 reports `07 07 00 01 02` (1,799 s, active, High). Low similarly
reports `1F 1C 00 01 01` (7,199 s). Changing the level during drying sends
the same command and restarts that level's countdown. An inactive Low
selection survives an MCU High report (03), then starts Low drying (04).
These are MCU reports, not physical observations or stock-app traffic.

Models affected by #176: `SUPERIOR6000S`; hardware evidence is limited to
LEH-S602S-WUS / MCU 1.0.9. The tag 00 fallback has synthetic tests only;
no other MCU version, full drying cycle, or Humidity-mode transition was tested.

## Status acknowledgements

[status_acks.txt](status_acks.txt) compares the old ACK with the protocol-byte
fix on LEH-S602S-WUS / MCU 1.0.9. Capture settings match the Dry follow-up.
The old exchange repeats status sequence 6C and gets `52 ... 35` replies.
Changing only ACK protocol `01` to `02` gives 15 status pushes with
15 distinct sequences, each received once, and no type-52 replies in the
recorded window. Power on/off, High Dry countdown and return to Manual/off
were checked through MCU reports. No sequence-counter or reserved-byte
change was needed: ACKs still use the ESP counter and byte 4 = FF.
