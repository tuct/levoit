# LV600S UART captures

Captured on one assembled EU LUH-A602S-WEUR with an ESP32-SOLO-1C, ESPHome
on the ESP32, and the original MCU firmware 1.0.56. The ESP logged both
directions of the appliance link at 9600 baud, 8N1.

Each CSV row contains a selected, complete frame from the original logs.
`TX` is ESP-to-MCU. `RX` is MCU-to-ESP. `elapsed_ms` is relative to the first
row of each case and uses the ESP UART log timestamp. Multiple frames in the
same log chunk can share a timestamp. The extracts omit intervening frames.
Frame bytes, including the sequence, length and checksum, are unchanged.
The [frame format](../../../LEVOIT_UART.md#frame-format) and the
[LV600S status offsets](../PROTOCOL.md#status) explain how to read them.

| Capture | Case | What to check |
|---|---|---|
| [poll-standby.csv](poll-standby.csv) | `standby-polls` | `0x4110` replies contain 20 status bytes. Two request/reply pairs show an echoed sequence and an incremented sequence. Empty `0xA265` replies provide no timer value. |
| [panel-mode-timer.csv](panel-mode-timer.csv) | `tank-states` | Status offsets 4 and 5 are `00/01` with the empty tank installed, `01/00` with the tank removed, and `00/00` after refill. |
| [panel-mode-timer.csv](panel-mode-timer.csv) | `panel-warm-mist`, `panel-mist` | The MCU initiates `0x4111` reports with control byte `0x02` during panel changes. Offsets 12 and 13 show mist settings 1, 5 and 9 and warm settings 0 through 3. The surrounding traffic in these cases contains ordinary polls. |
| [panel-mode-timer.csv](panel-mode-timer.csv) | `humidity-sleep-manual` | `0x4114` sets a 55% target in Humidity. `0xA2E8` changes it to 60%. `0x4082` selects Sleep with 60%, and `0xA260` selects Manual. Status offsets 8 and 11 show target and mode. |
| [panel-mode-timer.csv](panel-mode-timer.csv) | `timer-expiry` | `0xA264` sets 60 seconds. The `0xA266` report goes from 60 remaining and 60 total to zero remaining and 60 total. The last status push reports standby. |
| [panel-mode-timer.csv](panel-mode-timer.csv) | `panel-button-hold` | The MCU sends `0xD102` with action 2, then `0xD103` with action 4 about 10 seconds later. |
| [api-control-feedback.csv](api-control-feedback.csv) | `power-display-mist-warmth` | The `0xA000`, `0xA105`, `0x4113` and `0x4112` requests receive empty acknowledgements and separate status reports. Mist and warm levels are confirmed by status offsets 12 and 13. The display setting was already 100 when resent in this extract. |

The available complete status captures from this unit have 20-byte payloads.
The original ESP firmware's cold-start handshake, the read-only original Auto
mode, and nonzero fault codes are outside these extracts.
