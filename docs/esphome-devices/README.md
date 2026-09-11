# esphome-devices.com submission drafts

Device pages prepared for **[devices.esphome.io](https://devices.esphome.io)**
(repo: [esphome-devices/esphome-devices](https://github.com/esphome-devices/esphome-devices)).

Each folder here mirrors the site's own layout — `src/docs/devices/{Name}/index.md`
plus sibling YAML and images — so a folder can be copied across unchanged.

> **One new device per pull request.** The site's contribution rules require it;
> multiple new devices in one PR get bounced. Updates to existing pages may be
> grouped when they are the same kind of change.

## New pages

| Folder | Device | Status |
|--------|--------|--------|
| `Levoit-Core-200s` | Levoit Core 200S | ready |
| `Levoit-Core-600s` | Levoit Core 600S | ready — needs a device or UI image |
| `Levoit-Vital-100s` | Levoit Vital 100S | ready — needs a device or UI image |
| `Levoit-Vital-200s` | Levoit Vital 200S (Pro) | ready — needs a device or UI image |
| `Levoit-Everest-Air` | Levoit Everest Air | ready — needs a device or UI image |
| `Philips-Series-600` | Philips / MUJI Series 600 | ready |
| `Philips-Series-900` | Philips Series 900 | **hold** — see below |

## Updates to existing pages

These three are already live and were written against the old `projects/` layout in
the repo's previous name. The drafts here repoint them at `devices/`, add
`project-url`, and fix a few inaccuracies. Submit as one grouped PR.

| Folder | Change |
|--------|--------|
| `Levoit-Core-300s-enhanced` | links → `tuct/levoit`; drops the incorrect "Tuya MCU" description |
| `Levoit-Core-400s` | full content refresh — real feature list, `config.yaml`, teardown/flashing notes |
| `Levoit-Mini` | links → `tuct/levoit`; typo and wording fixes |

## Why Series 900 is on hold

ESPHome does not control that purifier yet — the MCU protocol is undecoded and the
`philips` component has no `AC0950`/`AC0951` model. Its `config.yaml` is a passive
UART sniffer, not a control config. The page is written and accurate, but it should
only go in once the device actually runs ESPHome, or be offered to the maintainers
as a research page if they want it earlier.

## Conventions worth remembering

* **No `!secret` anywhere** in the YAML, and no top-level `api:`, `ota:`, `mqtt:`,
  `web_server:`, `improv_serial:` or `bluetooth_proxy:`. `config.yaml` also must not
  carry `ssid`, `password`, `networks`, `manual_ip`, `eap` or `use_address` — the
  `wifi:` block is just a bare `ap:`.
* The first `file=` fence on a page must reference `config.yaml`, and the fence body
  stays empty — the site inlines the file.
* Valid `board` values are `bk72xx`, `esp32`, `esp8266`, `ln882x`, `rp2040`,
  `rtl87xx`. There is **no `esp32c3`** — C3 devices declare `esp32`.
* Valid `type` values are `dimmer`, `light`, `misc`, `plug`, `relay`, `sensor`,
  `switch`. Purifiers are `misc`.
* `difficulty` runs 1 (pre-flashed) to 5 (chip replacement).
* `alias` entries as bare strings are redirect-only — they keep a model code such as
  `Philips-AC0650` resolving without advertising it as the device name.

Every `config.yaml` here validates with `esphome config` against ESPHome 2026.7.0,
with the component pulled from the repo's `main`.
