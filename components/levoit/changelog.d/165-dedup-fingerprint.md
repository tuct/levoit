UART frame handling
the status-push dedup uses a 32-bit FNV-1a payload fingerprint instead of an
8-bit XOR/rotate one, which matched about 1 in 256 changed payloads and skipped
them. Affects all models using status-push dedup; based on the Core 300S
[`status_32_40.txt`](../../devices/levoit-core300s/uart/status_32_40.txt) ("speed 3")
