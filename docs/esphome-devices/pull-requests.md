# Pull request bodies

9 pull requests, one per branch — the site requires a single new device per PR.
All branches are pushed to `tuct/esphome-devices` and based on `upstream/main`.

| PR title | Branch | Open |
|----------|--------|------|
| Add Levoit Core 200S | `add-levoit-core-200s` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-core-200s) |
| Add Levoit Core 600S | `add-levoit-core-600s` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-core-600s) |
| Add Levoit Vital 100S | `add-levoit-vital-100s` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-vital-100s) |
| Add Levoit Vital 200S | `add-levoit-vital-200s` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-vital-200s) |
| Add Levoit Everest Air | `add-levoit-everest-air` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-everest-air) |
| Add Philips Series 600 Air Purifier | `add-philips-series-600` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-philips-series-600) |
| Add Philips Series 900 Air Purifier | `add-philips-series-900` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-philips-series-900) |
| Update Levoit Core 400s - full content refresh | `update-levoit-core-400s` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:update-levoit-core-400s) |
| Update Levoit device pages - repoint source links to the current repo layout | `update-levoit-repo-links` | [compare](https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:update-levoit-repo-links) |

The repo's template says not to delete anything from it, so each body below keeps
every section. Copy the block, paste it into the PR description, and the checkboxes
come pre-ticked.


---

## `add-levoit-core-200s`

**Title:** Add Levoit Core 200S

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-core-200s>

````markdown
# Brief description of the changes

Adds the Levoit Core 200S, a compact 3-speed air purifier with no air-quality sensor.

The purifier's Wi-Fi module and its control MCU are separate chips on a 115200 8N1 UART link, so
ESPHome replaces the stock firmware on the Wi-Fi ESP32 and drives the MCU through the `levoit`
external component. Tested against MCU firmware 2.0.11 on the stock ESP32-SOLO-1C.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `add-levoit-core-600s`

**Title:** Add Levoit Core 600S

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-core-600s>

````markdown
# Brief description of the changes

Adds the Levoit Core 600S — 641 m³/h CADR, four fan speeds, four auto profiles and a Luftme LD15
particulate sensor.

Same arrangement as the other Levoit units: the Wi-Fi module and the control MCU are separate chips
on a 115200 8N1 UART link, driven through the `levoit` external component. Tested against MCU
firmware 2.0.1. The page notes that retail units have shipped with different ESP32 modules, so the
`esp32:` block may need adjusting.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `add-levoit-vital-100s`

**Title:** Add Levoit Vital 100S

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-vital-100s>

````markdown
# Brief description of the changes

Adds the Levoit Vital 100S — four fan speeds, a PM1003 particulate sensor and a Pet preset.

The Wi-Fi module and the control MCU are separate chips on a 115200 8N1 UART link, driven through
the `levoit` external component. Tested against MCU firmware 1.0.5 on the stock ESP32-C3-SOLO-1.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `add-levoit-vital-200s`

**Title:** Add Levoit Vital 200S

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-vital-200s>

````markdown
# Brief description of the changes

Adds the Levoit Vital 200S (and 200S Pro, identical as far as the component is concerned) —
415 m³/h CADR, four fan speeds, PM1003 sensor and a Pet preset.

Same protocol as the Vital 100S, driven through the `levoit` external component. Tested against MCU
firmware 1.0.5 on the stock ESP32-C3-SOLO-1.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `add-levoit-everest-air`

**Title:** Add Levoit Everest Air

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-levoit-everest-air>

````markdown
# Brief description of the changes

Adds the Levoit Everest Air — 612 m³/h CADR, a 3-channel particulate sensor reporting PM1.0,
PM2.5 and PM10, and a motorised vent louver exposed as a number.

The Wi-Fi module and the control MCU are separate chips on a 115200 8N1 UART link, driven through
the `levoit` external component. Tested against MCU firmware 1.0.2 on the stock ESP32-SOLO-1.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `add-philips-series-600`

**Title:** Add Philips Series 600 Air Purifier

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-philips-series-600>

````markdown
# Brief description of the changes

Adds the Philips / Versuni Series 600 air purifier, sold under the MUJI brand. This is the first
Philips purifier page on the site.

Unlike the Levoit units this one cannot be reflashed — the stock module is an ESP32-C3-WROOM-02U
with secure boot enabled and enforced. The conversion is to wire in your own ESP32-C3 on the MCU
UART and park the original module by holding its `EN` pin low, which is fully reversible. The
`philips` external component implements the MCU's `FE FF` framed binary protocol.

Two builds exist and speak an identical protocol; the sensor model adds a PM1003 sensor, an allergen
index and an Auto preset, covered by the second `file=` fence. Tested against MCU firmware 0.1.9 and
0.2.1.

The `alias` entries keep the internal AC0650/AC0651 model codes resolving without advertising them,
since the unit is not sold under those names.

Both yaml files validate with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `update-levoit-core-400s`

**Title:** Update Levoit Core 400s - full content refresh

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:update-levoit-core-400s>

````markdown
# Brief description of the changes

Brings the Levoit Core 400s page up to the standard of the newer device pages.

- Adds a `config.yaml` — the page previously had no configuration at all
- Replaces the generic feature list with the entities the `levoit` component actually exposes on
  this model, including the PM2.5/AQI sensors, the auto-mode select and the room-size number
- Adds the tested MCU firmware version, board revision and particulate sensor part
- Adds flashing notes and a Home Assistant screenshot
- Repoints the source links from the repo's old `projects/` layout to the current `devices/` layout
- Corrects the description: the MCU is not a Tuya MCU

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [ ] New device (a single device only — one device per pull request)
- [x] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [ ] Adding a new device adds a single device only — one device per pull request. — n/a, this PR adds no device.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````


---

## `update-levoit-repo-links`

**Title:** Update Levoit device pages - repoint source links to the current repo layout

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:update-levoit-repo-links>

````markdown
# Brief description of the changes

The Levoit Core 300s enhanced and Levoit Mini pages both link to `tuct/esphome-projects` under a
`projects/` path. That repo has since been renamed and restructured; the links only still resolve
because of GitHub's rename redirect, and they point at the old unmaintained folder rather than the
maintained one.

- Repoints both pages at `tuct/levoit` under `devices/`
- Adds `project-url` frontmatter to both
- Core 300s enhanced: corrects the description — the MCU is not a Tuya MCU — and links the `levoit`
  component directly
- Levoit Mini: fixes two typos and clarifies that the original PCB is bypassed rather than modified

Grouped as a single PR since this is the same cleanup applied to two pages. No configuration
changes.

## Type of changes

- [ ] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [x] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [ ] Adding a new device adds a single device only — one device per pull request. — n/a, this PR adds no device.
- [ ] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages. — n/a, this PR changes no yaml.
- [ ] The first `file=` fence on the page references `config.yaml`. — n/a, these pages have no configuration.
- [ ] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [ ] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [ ] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, these pages are not made-for-esphome.
````

---

## `add-philips-series-900`

**Title:** Add Philips Series 900 Air Purifier

**Open PR:** <https://github.com/esphome/devices.esphome.io/compare/main...tuct:esphome-devices:add-philips-series-900>

````markdown
# Brief description of the changes

Adds the Philips / Versuni Series 900 air purifier.

The stock Wi-Fi module is an MXCHIP EMC6069-P, not an ESP32 and not a LibreTiny target, so it cannot
be reflashed. The conversion wires an ESP32-C3 onto the MCU's UART and parks the stock module by
holding its reset pad low; the module stays fitted and nothing is cut, so it is reversible.

The MCU link turned out to be the same `FE FF` protocol the `philips` component already spoke for the
Series 600, decoded from logic-analyzer captures with every write frame checked against them.

Verified on an AC0951 running MCU firmware 0.3.3. The AC0950 shares the configuration with
`model: AC0950` and without the particulate entities, but has not been observed on hardware — the
page says so rather than implying it is tested.

The `alias` entries keep the internal AC0950/AC0951 model codes resolving without advertising them,
since the unit is not sold under those names.

`config.yaml` validates with `esphome config` on ESPHome 2026.7.0.

## Type of changes

- [x] New device (a single device only — one device per pull request)
- [ ] Update existing device
- [ ] Removing a device
- [ ] General cleanup
- [ ] Other


## Checklist:

The rules below are enforced in CI by `npm run validate-devices` and `npm run validate-yaml`. The full reference is at [Configuration YAML files](https://devices.esphome.io/devices/adding-devices#configuration-yaml-files).

- [x] Adding a new device adds a single device only — one device per pull request.
- [x] Each example yaml lives in its own `.yaml` file alongside `index.md` and is pulled into the page with a fenced block of the form `` ```yaml file=<name>.yaml `` — no inline yaml on added or modified pages.
- [x] The first `file=` fence on the page references `config.yaml`.
- [x] `config.yaml` is **hardware-only**: no top-level `api:`, `ota:`, `mqtt:`, `web_server:`, `web_server_idf:`, `improv_serial:`, `captive_portal:`, `bluetooth_proxy:`, or `dashboard_import:`, and no `platform: homeassistant`, `platform: mqtt`, or `platform: template` anywhere in the tree.
- [x] If `config.yaml` has a `wifi:` block, it contains only radio tunables (`country`, `power_save_mode`, `output_power`, …) — no `ssid`, `password`, `networks`, `manual_ip`, `eap`, or `use_address`. An empty `ap:` block is allowed.
- [x] No passwords (literal **or** `!secret`) on `password:`, `*_password:`, or `psk:` keys, and no `!secret` references anywhere in any example yaml.
- [ ] For pages with `made-for-esphome: true` in frontmatter: at least one `` ```yaml url=… `` fence points at a `.yaml` file in the manufacturer's GitHub, Codeberg or GitLab repo — n/a, this page is not made-for-esphome.
````
