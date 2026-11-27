Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)), step 3b: the protocol
core lives in namespace `levoit`, free of any ESPHome name, and the ESPHome
component in `esphome::levoit_component`. Nothing changes for an ESPHome
build; a YAML lambda that spelled out `esphome::levoit::` would need the new
name
