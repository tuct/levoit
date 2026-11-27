Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)), step 2: the protocol
core owns the device state. `levoit_state.h` holds what the MCU reported, or
what was last requested, in the typed slots the decoders and builders already
use, and a `LevoitHost` interface hands a changed slot to the ESPHome entities
or, later, to Matter. Nothing changes in Home Assistant; a command builder now
echoes a reported value for a field whose entity is not configured
