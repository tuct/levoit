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
