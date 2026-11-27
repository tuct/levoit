[← Back](./README.md)

# Levoit Core 600S — hardware and protocol notes

## Protocol Notes

Same UART protocol as Core 300S/400S with these differences:

- Status push: `CMD=01 40 41` (vs `B0 40` / `30 40` on 300S/400S)
- ACK uses `0x52` response type (not `0x12`) with trailing byte `0x01`
- Light Detect command: `CMD=01 E9 A5` PAY=`01`/`00`
- 4 Auto Mode options: Default (`00`), Quiet (`01`), Room Size (`02` + room_size LE), ECO (`03`)
- Status byte layout: display `[8]`, AQI `[11]`, PM2.5 `[12-13]`, child lock `[14]`, auto mode `[15]`, room size `[16-17]`, light detect `[21]`
- Room size encoding: raw = sq_ft × 3.15 (MCU uses scaled sq ft)
