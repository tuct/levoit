Levoit UART frame handling
status-push dedup now uses a 32-bit fingerprint to reduce collisions: the 8-bit fingerprint
behind the dedup matched about 1 in 256 changed payloads and dropped them (Core 300S:
AQI and PM2.5 changing together, frames in
[`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)). Now 32 bits, all models
