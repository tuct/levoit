Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)): the plan in
[docs/shared-core.md](./docs/shared-core.md), and its first step: the protocol
sources log through `levoit_log.h` (ESPHome's, ESP-IDF's or a host's log),
`fan_modes.h` no longer needs ESPHome, and a new *Host tests* workflow builds
the frame layer with a plain `g++` against captured frames. Nothing changes in
an ESPHome build
