[← Back to the project README](../README.md)

# Firmware updates from Home Assistant — plan

> **Status: plan.** Phase 1 is merged ([#98](https://github.com/tuct/levoit/pull/98));
> the rest is not implemented yet. This page describes the target design and
> the order it gets built in. Comment in an issue before starting on a part of
> it.

Today every user builds their own firmware from a device YAML that pulls the
component with `ref: main`
([`devices/packages/source_git.yaml`](../devices/packages/source_git.yaml)) and
finds out about new releases by watching this repo.

The goal: Home Assistant **lists every device that runs the `levoit` or
`philips` component**, links each one to **its config in the ESPHome Device
Builder**, and shows when a **newer component version** is published. Installing
it compiles and uploads that config. This works the same way for every device:

| Hardware | Config | Built by |
|----------|--------|----------|
| The device's **original ESP** (Core 200S/300S/400S/600S, Everest Air, Sprout, Superior 6000S, Vital 100S/200S, Eleva 300X, Philips 600/900 …) | **Ready-made** from this repo: board, pins, model and entities filled in; the user only adds a **name** and **Wi-Fi credentials** | The user's ESPHome Device Builder |
| An **additional ESP** (XIAO C3/S3/C6, Ethernet boards, own wiring and pins) | The user's own config | The user's ESPHome Device Builder |

The original ESP is not a separate track. Its config is an ordinary user config,
with everything filled in except the name and the Wi-Fi.

## Goals

* Home Assistant lists every device running `levoit` or `philips`, on an
  original or an additional ESP, together with the Device Builder config it is
  built from
* **"Update available"** with release notes when a newer component version is
  published; installing compiles and uploads that config in the Device Builder
* Setting up an original ESP means picking the model and entering a name and
  Wi-Fi credentials, with no YAML editing
* A **channel** per device: `stable` (tagged releases) or `main` (every merge)
* **One version number** everywhere: the release tag, the component, the
  `ESP Version` sensor and the Home Assistant integration
* Later: flash a **stock** device without opening it, through a DNS add-on that
  serves the user's ESPHome build in place of the VeSync OTA
* A way **back to the original VeSync firmware**

## Non-goals

* **No prebuilt firmware images** and no `http_request` updates from GitHub
  Pages. Every device is built from its config in the user's Device Builder, so
  a device never runs a build that its config does not describe
* No custom update protocol on the device. The integration works through
  Home Assistant's ESPHome integration and the Device Builder
* Not a replacement for the Device Builder: editing configs stays there

## Building blocks already in ESPHome

Checked against ESPHome 2026.11 and the minimum 2026.5.3 unless marked
*to verify*:

* **Build records.** The Device Builder keeps one per config
  (`.esphome/storage/<config>.yaml.json`) with `name`, `address`,
  `esphome_version` and `loaded_integrations`. External components are listed
  there too (a `levoit` test build lists `levoit`), so the records tell which
  configs use `levoit` or `philips`, also for devices that are offline
* **Device ↔ config.** Home Assistant's ESPHome integration matches its devices
  to Device Builder configs by device name, and its firmware update entity
  installs by asking the Device Builder to compile and upload the config.
  **The integration uses the Device Builder API for everything**: listing
  configs, creating and changing a config, compiling and uploading. It never
  writes into the config folder directly, so it works the same wherever the
  Device Builder runs (add-on or its own container). *To verify:* the
  endpoints for listing, creating and saving a config, how the integration
  reaches and authenticates to the Device Builder outside the add-on, and
  whether it can reuse the client library Home Assistant uses
* **Remote packages** (`packages: github://tuct/levoit/<path>@<ref>`) let a
  short user config pull the ready-made device config from this repo. *To
  verify:* relative `!include` inside a remote package on 2026.5.3
* **`dashboard_import` + `esphome.project`**: a device running a ready-made
  config shows up as *discovered* in the Device Builder; *Adopt* writes a short
  config with the name, the package URL and `!secret wifi_ssid` /
  `wifi_password`, which is exactly the "add a name and Wi-Fi" step

## Design

### 1. One version, from one place — done

`components/levoit/VERSION` is the single source. It lives in the component
folder, so it ships with the component wherever `external_components` pulls it
from ([#98](https://github.com/tuct/levoit/pull/98)):

* the component reads it in `__init__.py` and compiles it in as
  `LEVOIT_VERSION`; `get_version()` reports it (`"1.7.0 esphome"`)
* a `Version check` workflow requires it to match the newest released change
  log entry on every PR, and the tag on release tags

Still to add:

* **The commit.** When the component comes from git, `__init__.py` also
  compiles in the commit it was built from, so a build from `main` between
  releases reports e.g. `1.7.0+3fa21c4`. Without it, every `main` build after
  1.7.0 looks like 1.7.0 and the integration cannot tell it is behind
* the integration's `manifest.json` carries the same version; CI fails if the
  tag, `VERSION` and `manifest.json` disagree
* **`philips` reports the same version.** This is a mono repo with one release
  number. `components/philips/__init__.py` reads `../levoit/VERSION`, which is
  always there because `external_components` clones the whole repo, and
  compiles it in. `philips` has no version sensor yet; it gets an
  `esp_version` text sensor like `levoit`. Its changes go into the same
  release notes

### 2. Ready-made configs for original ESPs

Every device folder gets a package that holds the whole device: board, pins,
UART, the component with its model, the entities, `esphome.project`
(`tuct.levoit-core300s`) and `dashboard_import`. The config the user keeps is
short:

```yaml
substitutions:
  name: levoit-core300s
  friendly_name: Bedroom purifier

packages:
  device: github://tuct/levoit/devices/levoit-core300s/package.yaml@stable

esphome:
  name: ${name}
  friendly_name: ${friendly_name}

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: "…"   # generated when the config is created
```

The `@stable` / `@main` at the end of the package URL is the channel.

This replaces the `*-builder.yaml` files and
[`make-builder-yaml.ps1`](../devices/make-builder-yaml.ps1), which inline
everything because relative `!include` does not work in the Device Builder; a
remote package does not need it. The existing full device YAMLs stay as the
example for additional ESPs and for local development.

Ways to get a ready-made config:

* **Adopt** a device that already runs one: it shows up as *discovered* in the
  Device Builder
* **Create** one from the integration (phase 4): pick the model, enter a name
  and Wi-Fi credentials (or use the Device Builder's `secrets.yaml`)
* **Paste** the snippet above into a new config in the Device Builder

The first flash of a stock unit is still over USB, from the Device Builder or
ESPHome Web, until the DNS add-on (phase 6) exists.

### 3. Release flow

| Trigger | What happens |
|---------|--------------|
| Tag `X.Y.Z` | GitHub release with the change log entry as notes; the workflow moves the `stable` branch to the tag |
| Merge to `main` | Nothing extra: `main` is the `main` channel |
| Every PR | CI compiles every ready-made device config (on top of the current compile tests) |

No firmware binaries are published.

### 4. Home Assistant integration "Levoit ESPHome" (HACS)

Lives in this repo at `custom_components/levoit_esphome/`, with a `hacs.json`
at the root. HACS requires the integration under `custom_components/` at the
repo root and allows one integration per repo; it ignores everything else
(`components/`, `devices/`). HACS takes its versions from this repo's GitHub
releases, so the integration shares the release number with the component.

What it does:

* **List:** every ESPHome device whose Device Builder config loads `levoit` or
  `philips` (from the build records), plus devices without a config there that
  report the component through the `ESP Version` sensor or a `tuct.*` project
  name. Per device: the config file (opens it in the Device Builder), original
  or additional ESP, channel, installed and latest version
* **Connect a config:** a device without a Device Builder config shows
  *no config*. For an original ESP the integration offers to create the
  ready-made config through the Device Builder API (model from the project
  name, name from the device, Wi-Fi from `secrets.yaml` or entered); for an
  additional ESP it links to the Device Builder to create one
* **Update entity per device:** installed version against the latest version of
  the device's channel (newest release for `stable`, newest commit for `main`),
  release notes from the GitHub release. *Install* asks the Device Builder to
  compile and upload that device's config
* **Channel:** shown per device; switching rewrites the `@stable` / `@main` ref
  in the config through the Device Builder API. Older configs that use `external_components` with `ref: main`
  show up on the `main` channel

### 5. Backup and restore page

A page on GitHub Pages, using ESP Web Tools / esptool-js, for the first USB
flash of an original ESP: **back up the stock flash first** and keep the file
locally, then flash the factory image the user downloaded from their own Device
Builder. Restoring is the same page with the backup file (see phase 7).

### 6. Later: DNS add-on for stock devices

A Home Assistant add-on that answers DNS for the VeSync firmware host and serves
the user's own ESPHome build (from the Device Builder) in its place, so a stock
unit can be flashed without opening it — the approach
[nico202/levoit-core300s-ota-wifi-flash](https://github.com/nico202/levoit-core300s-ota-wifi-flash)
takes for the Core 300S.

What is known from the Eleva 300X:

* the stock OTA downloads from **`fw.vesync.com`**, looked up by hostname
  (`failed to resolve host=fw.vesync.com` in the UART log when DNS fails)
* the full download URL is **not** in the firmware or the UART log; the cloud
  sends it with the update command (MQTT). The firmware has both `http://` and
  `https://` handling and a certificate for `*.vesync.com`
* the downloads are **public, plain HTTP, no credentials**. URLs seen in other
  projects' issues all answer `200` without login, for example
  `http://fw.vesync.com/WiFiBTOnboardingNotify_AirPurifier_Core300S_EU/mainFirmware/v2.0.00/Core300s_EU_ota_v2.0.00.rel.bin`
  and
  `http://fw.vesync.com/WFON_AHM_LUH-D301S-WUSR_US/v1.0.03/LUH-D301S-WUSR_WIFI_C1_ota_v1.0.03.rel.bin`.
  The pattern is `<configModule>/[mainFirmware|mcuFirmware/]v<version>/<file>`;
  a missing file answers `403`, so file names cannot be told apart from
  forbidden ones by probing
* the Eleva 300X (EU) is model **`LAP-C341S-WEU`**, configModule
  **`VS_WFON_APR_LAP-C341S-WEU_EU`**. The cloud's firmware info
  (`/cloud/v2/deviceManaged/getFirmwareUpdateInfoList`, as in nico202's
  `scripts/fetch_ota_info.py`; pyvesync 3.5.1 drops the Eleva from its device
  list, so the raw list has to be used) gives a newer path scheme:
  `http://fw.vesync.com/firmwareUpload/<configModule>/<mainFw|mcuFw>/v1/<file>`
  * ESP: `…/mainFw/v1/LAP-C341S-WEU_WIFI_C1_ota_v1.0.08.rel.bin` (1 187 009 B)
  * MCU: `…/mcuFw/v1/LAP-C341S-WEU_MCU_C1_ota_v1.0.13.bin` (64 284 B)
  * older versions are removed from the server (1.0.07 answers `403`)
* **The package header differs from the Core 300S.** The Core 300S `.rel.bin`
  is a 128-byte plain-text header (uppercase MD5 of the payload, name, version,
  region; see nico202's `make_ota.py`) + the ESP app image. The Eleva's is
  **129 bytes: `01` + 128 bytes that look random** (no MD5 or SHA-256 of the
  payload in plain form), followed by an app image byte-identical to the one the
  OTA wrote to `ota_1`. Either the header is encrypted (128 bytes = the old
  header size, AES-aligned) or it is a signature (RSA-1024 size). The firmware
  has `vesync_ota_get_header` / `vesync_ota_check_header`. **If it is a
  signature, the DNS route cannot work for devices on this newer VeSync SDK
  (v1.3.1)** — this has to be answered before building the add-on
* the stock partition table has two 1.5 MB app slots (`ota_0` / `ota_1`); the
  Eleva's ESPHome build is about 0.95 MB, so it fits

* the stock OTA checks an **MD5** sent along with the URL
  (`vesync_ota_check_md5`, `invalid md5:%s`); no signature check shows up in
  the strings, and Secure Boot is off
* the cloud connection is MQTT over TLS with username / password and a client
  certificate; the firmware download itself needs none (see above)

To research before building: what triggers the update (only the cloud?),
whether newer firmwares switch the download to HTTPS with a checked
certificate, and how to avoid the MCU pass-through update path.

### 7. Back to stock

Offering the original VeSync firmware back is part of the plan, so trying
ESPHome is not a one-way street. How depends on how ESPHome got onto the
device:

| Installed via | Stock firmware afterwards | Back to stock |
|---------------|---------------------------|---------------|
| DNS add-on (phase 6) | Still in the other app slot, together with `nvs` and the factory keys in `fac_data` | Point `otadata` at the stock slot and reboot — a button in the integration, no download |
| USB, with ESPHome's partition table | Overwritten | Write back the user's **own** flash backup (phase 5) |

Rules for the second case:

* **Only the device's own backup.** A full dump holds that unit's factory keys
  (`fac_data`, cloud identity), so a backup is never shared or reused on
  another unit
* **No VeSync binaries are hosted here.** The stock firmware is VeSync's; the
  repo only provides the tooling to take and restore a backup. The current
  stock app can also be fetched from VeSync's public URL (see phase 6): after
  stripping the package header it is the exact app image. That alone does not
  restore a device, though — the stock bootloader, partition table, `nvs` and
  the unit's `fac_data` have to be there too, which is why the device's own
  backup stays the restore path

For the DNS route, the ESPHome build has to keep the stock partition table
(two 1.5 MB slots) rather than bring its own, so the stock slot and `fac_data`
stay untouched.

### Help wanted: stock firmware URLs per model

Both the DNS route and "back to stock" need the stock firmware's download URL
for each model. The VeSync cloud only answers for Wi-Fi devices that are in the
asking account, so this needs owners. The request also takes a
`configModelList` of plain configModule strings (that is how the VeSync app
6.0.10 builds it, in `com.vesync.firmware.cloud.UpgradeCheckRequest`), but the
cloud only answers it for Bluetooth devices, which have no cid — the app's own
value `VS_BT_MTM_R161SB_ONL_US` works, the Eleva's
`VS_WFON_APR_LAP-C341S-WEU_EU` is rejected as `illegal argument`. Repeated
queries are rate-limited (`device or ip request is too frequent`).

**If a Levoit unit is still in your VeSync app**, run
[`tools/vesync_firmware_info.py`](../tools/vesync_firmware_info.py) and post
the output in an issue:

```
pip install pyvesync
python tools/vesync_firmware_info.py
```

It asks for your VeSync e-mail, password and the account's country code
(`DE`, `AT`, `US`, …; try `US` if the login fails), lists your devices with
`deviceType`, `configModule` and the installed / latest firmware, and checks
the file server for the matching downloads. The output contains no e-mail,
device ID or MAC address. It is read-only: login, device list, firmware info,
and `HEAD` requests to `fw.vesync.com`.

Known so far:

| Model | Part | Version | URL |
|-------|------|---------|-----|
| Eleva 300X EU (`LAP-C341S-WEU`) | ESP | 1.0.08 | `http://fw.vesync.com/firmwareUpload/VS_WFON_APR_LAP-C341S-WEU_EU/mainFw/v1/LAP-C341S-WEU_WIFI_C1_ota_v1.0.08.rel.bin` |
| Eleva 300X EU (`LAP-C341S-WEU`) | MCU | 1.0.13 | `http://fw.vesync.com/firmwareUpload/VS_WFON_APR_LAP-C341S-WEU_EU/mcuFw/v1/LAP-C341S-WEU_MCU_C1_ota_v1.0.13.bin` |
| Core 300S EU | ESP | 2.0.00 | `http://fw.vesync.com/WiFiBTOnboardingNotify_AirPurifier_Core300S_EU/mainFirmware/v2.0.00/Core300s_EU_ota_v2.0.00.rel.bin` (from nico202) |
| Core 400S EU | ESP | 2.0.00 | `http://fw.vesync.com/WiFiBTOnboardingNotify_AirPurifier_Core400S_EU/mainFirmware/v2.0.00/Core400s_EU_ota_v2.0.00.rel.bin` (from nico202 #1) |

Only the current version of each file stays on the server; older ones answer
`403`.

## Order

| Phase | Deliverable | Depends on |
|-------|-------------|------------|
| 1 | `components/levoit/VERSION`, component define, CI consistency check ([#98](https://github.com/tuct/levoit/pull/98)) — **done** | — |
| 2 | Commit in the version string; `philips` reports the same version; release workflow (GitHub release, `stable` branch) | 1 |
| 3 | Ready-made package per original-ESP device, short adopt config, CI compiles them | 2 |
| 4 | HACS integration "Levoit ESPHome": list, connect a config, update, channel | 2, 3 |
| 5 | Backup and restore page | — |
| 6 | DNS add-on (research first) | 3 |
| 7 | Back to stock: restore on the page, slot switch for DNS installs | 5, 6 |

Phases 2 and 3 can ship before the integration: the Device Builder already
offers adoption, and users on `@stable` get releases with their next install.

## Decisions

* **One version for the repo.** `levoit` and `philips` report the same
  release number from `components/levoit/VERSION`
* **The integration goes through the Device Builder API**, never through the
  config folder
* **The integration keeps the name "Levoit ESPHome"**, although it also covers
  the Philips devices
