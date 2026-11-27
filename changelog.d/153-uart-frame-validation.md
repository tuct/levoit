* **Levoit UART frame handling.** Decode both length bytes, discard impossible
  or truncated frames, and preserve complete frames in long bursts.
  Core 600S and Vital 100S echo the incoming sequence in ACKs and reject
  frames with an unexpected protocol version or error result.
