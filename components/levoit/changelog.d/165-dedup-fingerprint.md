UART frame handling
the status-push dedup uses a 32-bit FNV-1a payload fingerprint instead of an
8-bit XOR/rotate one, which matched about 1 in 256 changed payloads and skipped
them. Host test with a Core 300S `01 32 40` frame whose AQI and PM2.5 changed
together
