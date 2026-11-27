#pragma once
// The protocol core lives in namespace levoit (docs/shared-core.md, 3b). The
// ESPHome side of the component lives in esphome::levoit_component: not
// esphome::levoit, because ESPHome's generated main.cpp says
// `using namespace esphome;` and a bare `levoit::` there would then be
// ambiguous. This makes the core's names visible on the ESPHome side.
namespace levoit {}
namespace esphome {
namespace levoit_component {
using namespace ::levoit;
}  // namespace levoit_component
}  // namespace esphome
