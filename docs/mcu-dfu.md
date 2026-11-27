[← Back to the project README](../README.md)

# Flashing the appliance MCU

The purifier has a second processor, the **SinOne SC95F8616** (8051 core,
16 KB flash), that drives the fan, sensors and panel. The ESP32 reaches it only
over the internal UART (`GPIO17` TX / `GPIO16` RX, 115200 8N1). The stock
firmware uses that link to update the MCU; `McuDfu` implements the same host
side so custom firmware can do it too.

This is the **appliance controller's** flash, not the ESP's. A wrong image
bricks the board and the ESP cannot recover it: there is no exposed ICP/SWD
header. Only use an image you have a known-good copy of.

## Wire format

Same framing as every other MCU message (`levoit_message.h`):

```
A5 | 22/12 | seq | len_lo | len_hi | checksum | 01 | op_lo | op_hi | 00 | payload
```

`checksum = 0xFF - (sum of all bytes except byte 5)`. The opcode is the 16-bit
`op_hi:op_lo` in the inner header. The DFU uses:

| opcode | direction | payload |
|---|---|---|
| `0x8030` start | ESP → MCU | `00 04 maj min patch size_le32 FF FF` |
| `0x8030` ACK | MCU → ESP | `status(0=ok) mtu_le16` |
| `0x8031` pull | MCU → ESP | `offset_le32` |
| `0x8031` ACK | ESP → MCU | `is_last offset_le32 len_le16 data…` |
| `0x8032` verified | MCU → ESP | `status` |
| `0x8033` burned | MCU → ESP | `status` |
| `0xD101` reboot | ESP → MCU | `01` |

The MCU **pulls**: after the start frame the ESP only answers `0x8031`
requests, so the transfer is flow-controlled by the MCU. The chunk size is the
MTU the MCU sends in its `0x8030` ACK (clamped to 64..1024; the stock firmware
advertises 512).

## Sequence

1. `start(reader, size, major, minor, patch)` sends `0x8030`; state
   `WAIT_READY`.
2. The MCU ACKs with status and MTU; state `TRANSFER`.
3. Each `0x8031 (offset)` reads that offset through the reader and answers
   with the chunk; `is_last` marks the final one.
4. `0x8032` (status 0) → ACK, state `BURNING`.
5. `0x8033` (status 0) → ACK, send `0xD101`, state `DONE`.
6. Ten seconds without progress → `FAILED`; the MCU also gives up, so a retry
   starts again at `0x8030`.

The stock firmware does the same from `vesync_ota_mcu_start` (`0x400eb184`) and
`ota_mcu_bin_requst` (`0x400eb040`), with the image in the `storage` partition.

## Using it

The image is a `Reader` callback, so the core does not care where it lives:

```cpp
// embedded image, generated with: xxd -i mcu_fw.bin > mcu_image.h
device_.mcu_dfu().start(
    [](uint32_t off, uint8_t *dst, size_t len) -> size_t {
      if (off >= kMcuImageSize) return 0;
      size_t n = std::min(len, kMcuImageSize - off);
      memcpy(dst, kMcuImage + off, n);
      return n;
    },
    kMcuImageSize, /*major=*/1, /*minor=*/0, /*patch=*/5);
```

The ESPHome glue can expose it as a button:

```yaml
button:
  - platform: template
    name: "Flash MCU"
    entity_category: config
    on_press:
      - lambda: id(levoitcore300).start_mcu_dfu(1, 0, 5);
```

with `Levoit::start_mcu_dfu()` supplying the reader. A reference implementation
with a host test that checks the exact frames is in this PR.

### Where the image can live

* **Embedded** (`xxd -i`): works on a fresh unit, costs the image size (≤ 16 KB)
  of flash. This is the only option that needs no prior download.
* **`storage` partition** (`0x340000`, label `storage`): what the stock firmware
  does. It downloads the MCU image there and serves it with
  `vhal_ota_save_bin_file`. ESP32-only; you need to record the size and version
  yourself (for example a small header at offset 0).
* NVS is too small for 16 KB, so it is not an option.

While a transfer runs, `LevoitDevice::sendCommand()` drops normal commands, as
the stock firmware does with its busy semaphore.
