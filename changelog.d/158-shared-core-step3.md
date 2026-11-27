Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)), step 3: the protocol is
a core of its own. `LevoitDevice` holds the UART framing, decoding, commands,
timers, CADR and filter tracking and the LEDs, and builds without ESPHome; the
ESPHome component feeds it the UART and implements the two interfaces it talks
through. A host test replays captured Core 300S frames through the real core
