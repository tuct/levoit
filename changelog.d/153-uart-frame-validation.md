Levoit UART frame handling
both length bytes are decoded, impossible or truncated frames are dropped, and
complete frames survive long bursts. Core 600S and Vital 100S echo the incoming
sequence in ACKs and reject frames with an unexpected protocol version or
error result
