# LV600S release review

Scope is the tested EU `LUH-A602S-WEUR`, original ESP32-SOLO-1C, 4 MB flash
and MCU `1.0.56`. The [hardware guide](HARDWARE.md) documents the three
disassembly and wiring photos, external isolated 5 V at CN1, 3.3 V UART
logic, the GPIO0 boot strap, private backups and first-boot verification.
The [protocol notes](PROTOCOL.md) record the observed frame fields, status
layout, commands, service codes and evidence limits.

## Code and dependencies

| Area | Change | Effect on shared code |
|---|---|---|
| ESPHome `components/lv600s_humidifier` | Separate fixed-layout UART transport, bounded framing and write queue, MCU-confirmed controls, timer and diagnostics. The release candidate discards pending setters when status becomes stale or unsafe and retains explicit off | Uses ESPHome UART, sensor, binary-sensor and text-sensor APIs. Reads the existing component version helper. Its parser, queue and status layout are separate from the `levoit` purifier TLV implementation |
| Development YAML | Adds an original-ESP example that builds against the local checkout | The stable catalogue and builder generator retain their existing scope |
| Tested secure owner profile | Preserves the original partition table, individual encrypted API and OTA access, stored Wi-Fi and owner reset policy. The mist, app-scale and warmth numbers read MCU values every second | These settings are in the LV600S profile of the owner repository. The original two purifier profiles use their own YAML and component |
| HA recognition and presentation | Adds the LV600S model and component, maps nested MCU diagnostic entities and German or English names, and renders dedicated controls. The release candidate excludes an auxiliary ESPHome fan from purifier controls and group discovery on a known LV600S | Existing device/entity IDs and the ESPHome transport are reused. Fan matching for other models is covered by a regression assertion. No custom humidifier platform is required |

The owner test branches also carry separate purifier UART/ACK validation and
HA template refresh protection for encrypted OTA. Those cross-model changes
are separate from the LV600S contribution branches. The HA owner profile uses
a custom local builder config, which avoids automatic template rewrites.

## Verification and scope

- The contribution compiled with all LV600S entities under ESPHome 2026.6.0
  and 2026.11.1. The development YAML validated with both versions. The secure
  owner candidate also compiled with its original partition table, existing
  keys and required encrypted OTA.
- The native protocol, command confirmation, timer and safety tests passed
  under Linux GCC with AddressSanitizer and UndefinedBehaviorSanitizer.
- On ESPHome 2026.10.2 all seven Levoit and Philips comparison profiles
  compiled. The prior owner preview resolved the component from the owner
  fork. The contribution uses local component sources for the development config.
  The upstream Builder generator check passed with the catalogue and existing
  generated YAML files unchanged.

- The assembled owner device ran `1.2.01-tuct-review` and `1.2.02-tuct-review`.
  Local power, display, mist, warm level, humidity target, modes, countdown and
  timer cancel were confirmed. Firmware `1.2.02-tuct-review` reported mist,
  app scale and warm level to HA within 1.00–1.12 seconds. HA detected 20
  controls and diagnostics. [Owner test results](https://github.com/mvs90/levoit/blob/codex/owner-test/docs/esphome-devices/owner-test.md)
- Earlier hardware tests with the prior implementation established absent,
  empty and filled tank reporting, a water-shortage transition to standby,
  timer expiry and physical display response. The newer queue correction was
  exercised with synthetic stale and unsafe transitions, including recovery
  in the same receive burst and a pending off command.
- On 26 November 2026 the owner LV600S accepted `1.2.03-queue-review` via
  encrypted OTA. With the empty tank installed and the unit in standby, the
  firmware rejected power-on, mist, warm level, mode, timer and display writes.
  The MCU confirmed an explicit off command. Settings were checked after the
  test. The tank-removal, filled-tank and physical reset checks of this build
  still need hands-on access.
- HA integration 0.6.8 identified the real device and mapped all 20 controls
  and diagnostics in an isolated HA 2026.10.4 instance. The native encrypted
  ESPHome connection supplied the states. The test used a simulated Device
  Builder listing and exercised a blocked ON and a confirmed OFF through HA.
- The release candidate uses the same entity names and command payloads.
  Original Auto is read-only. Heating and humidifying flags are derived from
  MCU status. Long-term behaviour, other board and MCU revisions, and the
  newest builds' fault and reset handling still need a device run.
- The development YAML uses the default ESPHome partition table. The already
  converted EU device uses the secure owner profile with its original table. [Secure owner build and live verification](https://github.com/mvs90/levoit-lv600s-local/blob/codex/tuct-alignment/docs/UPSTREAM-ALIGNMENT.md)

The [protocol evidence](PROTOCOL.md#recorded-command-events-and-water-depletion-payloads)
includes 33 command events and 144 observed MCU status payloads from the EU
unit. The native test replays the recorded water-depletion transition with
synthetic envelopes. The separate
[bidirectional standby capture](PROTOCOL.md#bidirectional-standby-capture)
contains 63 complete checksum-valid frames. All 32 received frames passed
local replay through the current decoder. It supports the status poll,
empty timer reply and OFF confirmation tests. Stock ESP startup and a
wet-run trace remain outside this evidence.
