UART frame handling
the receive loop decodes both length bytes, bounds the frame, drops a partial
frame after 250 ms of silence, and keeps complete frames in bursts longer than
the buffer. Core 600S and Vital 100S reject error and foreign-version envelopes
and echo the incoming sequence in ACKs; other models keep their ACK bytes.
Parser regression under ASan and UBSan in CI; tested on a Core 600S
(MCU 1.0.12) and a Vital 100S (MCU 1.0.5)
