[← Back to the project README](../README.md)

# One protocol core for ESPHome and Matter

The `levoit` component speaks to the purifier's MCU over UART and maps what it
hears to ESPHome entities. A Matter firmware ([#147](https://github.com/tuct/levoit/issues/147))
needs the first part unchanged and maps it to Matter clusters instead. This
page is the plan for sharing that first part, in steps that each leave ESPHome
users exactly where they were.

## What is shared and what is not

| | Shared | Per variant |
|---|---|---|
| Protocol | frames, checksum, ACKs; the status decoders and command builders of every model; the timers; CADR and filter tracking | |
| Mapping | | ESPHome: entities. Matter: fan, air quality, PM2.5 and filter clusters |
| Platform | | UART driver, saved values, timers, logging, OTA, pairing |

Today the protocol is only half separated. The decoders and builders
(`*_status.cpp`, `*_commands.cpp`, `decoder.cpp`, `tlv.cpp`, about 3,000
lines) hold no ESPHome code of their own and, since step 2, read and write a
device state that builds without ESPHome. But they still reach it through
`Levoit`, the ESPHome component in `levoit.h`, and that header pulls in
`Component`, `UARTDevice`, preferences and the clock. `levoit.cpp` (1,700
lines) mixes the protocol with that glue.

## Steps

### 1. Shims, and a host build that proves them (done, #155)

* `levoit_log.h` picks the log macros: ESPHome's, ESP-IDF's, or `stderr` on a
  host. The protocol files include it instead of `esphome/core/log.h`, and drop
  `esphome/core/defines.h`, which none of them used.
* `fan_modes.h` takes a plain string instead of ESPHome's `StringRef`.
* `tests/host/levoit_frame_test.cpp` builds the frame layer (`levoit_message.h`,
  `tlv.cpp`, `decoder_helpers.h`, `fan_modes.h`, `types.h`) with a plain `g++`
  and checks it against frames captured from a Vital 100S. The *Host tests*
  workflow runs it on every PR.

Nothing changes for an ESPHome build: the same macros expand, from the same
header.

### 2. The core owns the state; the host hears about changes (this branch)

The decoders already speak in typed enums rather than entities: about a
hundred `publish_*(SwitchType::…, NumberType::…, SensorType::…, …)` calls and
some thirty `try_get_*_state` read-backs over the same enums (`types.h`). So
`levoit_state.h` holds one typed `LevoitState`, a known-flag and value per
enum slot plus the fan's power/speed/mode, and a `LevoitHost` interface with
one method per kind of slot. `Levoit` owns the state and implements the host:

* `publish_*` records the slot and calls the host method only when the value
  changed. The host methods are the old `publish_*` bodies, entity dedup, room
  size rounding and select option checks included, so what reaches Home
  Assistant is unchanged. A Matter firmware implements the same seven methods
  and maps to clusters.
* `try_get_*` reads the state. A slot is known once the MCU reported it or a
  command requested it, whether or not the entity is in the YAML: the
  `on_*_command` entry points write the requested value first, as the
  optimistic entity publish did, and `LevoitNumber` reports its restored
  values at boot. For a builder this is the one visible change: a field whose
  entity is not configured now echoes what the MCU reported instead of a
  default.
* The decoders are untouched; they still take `Levoit *`, which step 3 turns
  into the core.
* `tests/host/levoit_state_test.cpp` builds the state with a plain `g++`.

One struct keyed by the existing enums, not one per model: a model fills the
slots it has. (Shape agreed with tadhglewis in
[#147](https://github.com/tuct/levoit/issues/147); forwarding entity-shaped
`publish_*` calls to the host, the first idea, would have made Matter rebuild
the state from them.) The Sprout's LED ring light (`publish_sprout_light`)
stays a separate call for now; it joins the state when the Sprout gets its
Matter mapping.

The platform interface, write bytes to the UART, milliseconds since boot, a
one-shot timer, load and save a `uint32` (used CADR, runtime), the local time
when there is a clock, and whether the network is up (for the Wi-Fi LED),
comes with step 3, where `LevoitDevice` is its first user.

### 3. Split `levoit.cpp`

* **Into a `LevoitDevice` core:** UART receive and framing, `process_message`,
  ACKs, `sendCommand`, the `on_*_command` mapping, the timers, CADR and filter
  tracking, the Eleva auto config and the bulk-prefs cache.
* **Left in the ESPHome `Levoit`:** a thin component that owns one
  `LevoitDevice` and wires up `setup`/`loop`, the UART, preferences and the
  entities.

After this step the host test can replay whole captured frames through the
decoders and check what a host receives.

### 4. Packaging for a Matter firmware

The core gets a `CMakeLists.txt` and `idf_component.yml`, so an ESP-IDF project
pulls it with the IDF component manager at a tag of this repo. A Matter
firmware then implements the two interfaces and keeps its own mapping layer,
which is what [levoit-matter](https://github.com/tadhglewis/levoit-matter)
already has in `purifier_map` and `matter_setup`. In return it gets the
protocol of every model here, not only the Core 200S to 600S, and later fixes
for free.

## To check before step 3

ESPHome copies the component's source files into its build. Whether the core
can live in a subfolder, and whether a `CMakeLists.txt` next to the sources
leaves ESPHome's build alone, needs a compile test first. The core may have to
stay at the top level of `components/levoit/`.

## Decisions and what is still open

* **Namespace:** the core moves to a neutral `levoit::` as part of step 3, not
  as a separate rename. The ESPHome side stays `esphome::levoit` because of
  ESPHome's codegen. levoit-matter renames its own `levoit` namespace first so
  the two do not collide (agreed in #147).
* **Philips:** a separate protocol in `components/philips`, same shape, later.
* **LV600S:** `components/lv600s_humidifier` (#152) speaks the same Levoit
  frame with a fixed-layout payload at 9600 baud. The goal is `model: LV600S`
  in `levoit` once the core exists.
