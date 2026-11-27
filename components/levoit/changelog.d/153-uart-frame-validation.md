* **UART frame handling**: the receive loop decodes both length bytes, bounds
  the full frame, drops a partial frame after 250 ms of silence, and works
  through bursts longer than the receive buffer without discarding frames that
  were already complete. Core 600S and Vital 100S reject error and
  foreign-version envelopes; their ACKs echo the incoming sequence and send
  the high length byte as zero. Other models keep their existing ACK bytes and
  transmit counter. `tests/test_uart_integrity.py` runs the real C++ parser on
  corrupt and truncated streams and on known ACKs from the Core 600S and Core
  300S traces, under ASan and UBSan in CI. Tested on a Core 600S (MCU 1.0.12)
  and a Vital 100S (MCU 1.0.5): power, speeds, modes, display, child lock,
  PM2.5 and timer start, countdown, cancel and expiry
