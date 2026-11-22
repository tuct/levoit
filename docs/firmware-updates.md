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

The goal: a Home Assistant add-on **lists every device that runs the `levoit`
or `philips` component** with its type, name, ESPHome version and component
version, connects each one to **its config in the ESPHome Device Builder**
(create, connect, manage), and makes moving to a **new component or ESPHome
version** one step: recompile every device, then upload. A small integration
adds Home Assistant update entities. This works the same way for every device:

| Hardware | Config | Built by |
|----------|--------|----------|
| The device's **original ESP** (Core 200S/300S/400S/600S, Everest Air, Sprout, Superior 6000S, Vital 100S/200S, Eleva 300X, Philips 600/900 …) | **Ready-made** from this repo: board, pins, model and entities filled in; the user only adds a **name** and **Wi-Fi credentials** | The user's ESPHome Device Builder |
| An **additional ESP** (XIAO C3/S3/C6, Ethernet boards, own wiring and pins) | The user's own config | The user's ESPHome Device Builder |

The original ESP is not a separate track. Its config is an ordinary user config,
with everything filled in except the name and the Wi-Fi.

## Goals

* One page that lists every device running `levoit` or `philips`, on an
  original or an additional ESP: type, name, ESPHome version, component
  version, and the Device Builder config it is built from
* Create, connect and manage that config in the Device Builder from the same
  page
* **New component or ESPHome version → recompile every device, then upload**,
  per device or all at once
* **"Update available"** with release notes in Home Assistant when a newer
  component version is published
* Setting up an original ESP means picking the model and entering a name and
  Wi-Fi credentials, with no YAML editing
* Later: **upload a `.bin`** to a device, at the user's own risk (for example a
  stock app image)
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
* No custom update protocol on the device. The add-on works through
  Home Assistant, the Device Builder and ESPHome's own OTA
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
  **The add-on uses the Device Builder API for everything about configs**:
  listing, creating and changing a config, compiling and uploading. It never
  writes into the config folder directly. *To verify:* the endpoints for
  listing, creating and saving a config, and how the add-on reaches and
  authenticates to the ESPHome add-on
* **Uploading a prebuilt image.** `esphome upload --file <app.bin> --device <ip>`
  (2026.11) sends an app image over native OTA without a config. It refuses
  factory images; `--partition-table` / `--bootloader` send those instead. Data
  partitions (`nvs`, `fac_data`) cannot be written over OTA. *To verify:* the
  same on 2026.5.3

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
  1.7.0 looks like 1.7.0 and the add-on cannot tell it is behind
* the integration's `manifest.json` carries the same version; CI fails if the
  tag, `VERSION` and `manifest.json` disagree
* **`philips` reports the same version.** This is a mono repo with one release
  number. `components/philips/__init__.py` reads `../levoit/VERSION`, which is
  always there because `external_components` clones the whole repo, and
  compiles it in. `philips` has no version sensor yet; it gets an
  `esp_version` text sensor like `levoit`. Its changes go into the same
  release notes

### 2. Ready-made configs for original ESPs

These exist already: the **builder files**. Every device config with a
`packages:` block has a self-contained `*-builder.yaml` next to it (for example
[`levoit-core300s-builder.yaml`](../devices/levoit-core300s/levoit-core300s-builder.yaml),
`-builder-c3`, `-builder-s3`), generated by
[`make-builder-yaml.ps1`](../devices/make-builder-yaml.ps1): the device header
(board, pins), the component from git, and `common.yaml` copied in, because
relative `!include` does not work in the Device Builder. They read
`wifi_ssid`, `wifi_password`, `wifi_ap_password`, `api_key` and `ota_password`
from `secrets.yaml`, and the name from the `device_name` substitution.

Phase 3 makes them fit for the add-on:

* **Channel** = the `ref:` of the component's git source. The script writes
  `ref: stable` (today `ref: main`); the add-on switches it by rewriting that
  line
* **Per-device keys.** All builder files read the same `!secret api_key` and
  `ota_password`. For a **new** device the add-on writes a fresh API
  encryption key and OTA password into its config, so devices do not share one
  key. A device that already runs ESPHome **keeps its own** (see onboarding in
  phase 4a)
* **Always in step with the sources.** CI regenerates the builder files and
  fails if they differ from the committed ones, then compiles them with dummy
  secrets
* the existing full device YAMLs stay as they are, for local development and as
  the example for additional ESPs

Ways to get a ready-made config:

* **Create** one from the add-on (phase 4): pick the model and the ESP variant,
  enter a name; Wi-Fi from the Device Builder's `secrets.yaml` or entered. The
  add-on fetches the builder file at the channel's ref and saves it through the
  Device Builder API
* **Paste** a builder file into a new config in the Device Builder, as today

A builder file is a full copy: releases bring new **component** code through
`ref:`, but changes to the device config itself (new entities in
`common.yaml`, a pin fix) only arrive when the config is created again. The
add-on can offer that as *refresh from template*, keeping the name, keys and
channel.

The first flash of a stock unit is still over USB, from the Device Builder or
ESPHome Web, until the DNS add-on (phase 6) exists.

### 3. Release flow

| Trigger | What happens |
|---------|--------------|
| Release `X.Y.Z` published (notes written by hand) | The `Release` workflow checks the tag against `VERSION` and moves the `stable` branch forward to it |
| Merge to `main` | Nothing extra: `main` is the `main` channel |
| Every PR | CI checks the builder files are regenerated and compiles them (on top of the current compile tests) |

No firmware binaries are published.

### 4. Add-on and integration "Levoit ESPHome"

Two parts with one name and one release number:

| Part | Does | Runs on |
|------|------|---------|
| **Add-on** | The device page: list, configs, recompile and upload | Home Assistant OS / Supervised, next to the ESPHome add-on |
| **Integration** (HACS) | Home Assistant update entities, install through the add-on | Every install; without the add-on it only shows that an update exists |

Both live in this repo: the add-on in `levoit_esphome/` with a
`repository.yaml` at the root (the repo becomes an add-on repository), the
integration in `custom_components/levoit_esphome/` with a `hacs.json` at the
root. HACS takes versions from this repo's GitHub releases. *To verify:* that
one repo can be an add-on repository and a HACS repository at the same time.

#### 4a. Add-on

A sidebar page (ingress). The add-on talks to Home Assistant (device registry
and entity states, through the Supervisor), to the Device Builder (its API, see
above).

**Device list** — every device running `levoit` or `philips`:

| Column | From |
|--------|------|
| Name | Home Assistant device / Device Builder config |
| Type | `model:` in the config (`CORE300S`, `AC0951` …) and the board; original or additional ESP |
| ESPHome version | The firmware running on the device (ESPHome integration) and the Device Builder's current version |
| Component version | The `esp_version` sensor (`1.7.0+3fa21c4`) against the newest release / commit of the device's channel |
| Config | Linked config file, or *no config* |
| Channel | `stable` / `main`, from the component's `ref:` in the config |

Found through the Device Builder's build records (`loaded_integrations`, also
for devices that are offline) and through Home Assistant devices that have an
`esp_version` sensor but no config.

**Configs** (all through the Device Builder API):

* **Connect** a device to an existing config (matched by device name, or picked
  by hand)
* **Create** a config: for an original ESP the ready-made one (model, name,
  Wi-Fi from `secrets.yaml` or entered); for an additional ESP a starting
  config to finish in the Device Builder
* **Manage**: open in the Device Builder, switch the channel (rewrites the
  `ref:`), rename, refresh from template (see phase 2)

**Onboarding an existing device.** Most devices already run ESPHome with a
config of their own. Moving one to a builder-file config must keep what the
device and Home Assistant already use, or the first upload breaks it: a new
OTA password makes the device refuse the upload, a new API key makes
Home Assistant lose the device, a new name makes it a new device.

| Kept | Read from |
|------|-----------|
| API encryption key | The device's current config, resolving `!secret` through the Device Builder's `secrets.yaml` (both read through the Device Builder API). No config there: the Home Assistant ESPHome config entry of the device (`noise_psk`), passed on by the integration, which runs inside Home Assistant |
| OTA password | The current config / `secrets.yaml`. Home Assistant does not store it, so without a config the user enters it (or the first install goes over USB) |
| Name (`esphome.name`, `friendly_name`) | The current config, else the Home Assistant device |
| Wi-Fi, fallback AP password, static IP / `use_address` | The current config; Wi-Fi otherwise from `secrets.yaml` or entered |
| Board / ESP variant | The current config or build record, checked against the builder file chosen |

Rules:

* **References stay references.** A value the old config reads with
  `!secret xyz` is written as `!secret xyz` again, not copied in as plain text
* **The old config is kept**: renamed to `<name>.yaml.bak` (or a copy saved)
  before the new one replaces it, so onboarding can be undone
* **Preview first**: the page shows the new config and what was carried over;
  nothing is saved or compiled before the user confirms
* **Validate before upload**: the new config is compiled first; a value that
  could not be read stops onboarding with a message instead of guessing

*To verify:* a device on an Arduino config moving to the ESP-IDF builder file
over OTA (the partition table differs; ESPHome 2026.11 can send one with
`--partition-table`).

**Updates.** A device is behind when a newer component version is out on its
channel, or when the Device Builder runs a newer ESPHome than the device. Then:

1. **Recompile** — the outdated devices, or all, one after another in the
   Device Builder; the page shows progress and the build log per device
2. **Upload** — per device or all compiled ones, after a confirmation. Compile
   and upload are separate steps, so a failed build never touches a device

Uploading an arbitrary `.bin` is not part of 4a; see phase 8.

#### 4b. Integration

* One **update entity** per device: installed component version, latest of the
  channel, release notes from the GitHub release
* **Install** asks the add-on to recompile and upload that device
* Without the add-on it compares the `esp_version` sensor with GitHub and only
  shows that an update exists
* **Onboarding without a config:** hands the add-on the API encryption key
  from the device's ESPHome config entry. Only for the device being onboarded,
  only when the user starts it. Until 4b exists, onboarding needs the device's
  config in the Device Builder

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
| DNS add-on (phase 6) | Still in the other app slot, together with `nvs` and the factory keys in `fac_data` | Point `otadata` at the stock slot and reboot — a button in the add-on, no download |
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

### 8. Later: upload a `.bin` — at your own risk

A page in the add-on: pick a device and an app image, and the add-on sends it
with `esphome upload --file` over native OTA (key / password from the device's
config). For example the stock app image from the user's own backup, or one
from VeSync's server with the package header stripped. Limits the page states
before the upload:

* app images only; ESPHome refuses factory images. A full flash dump
  (bootloader, partition table, `nvs`, `fac_data`) goes back over USB
  (phase 5)
* after the upload the device runs whatever the image is; if it is not
  ESPHome, the add-on cannot reach it any more
* a stock app on ESPHome's partition table does not find its `nvs` /
  `fac_data` where it expects them, so it may not work. It is meant for
  devices that kept the stock partition table (the DNS route, phase 6)

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
| 2 | Commit in the version string; `philips` reports the same version; release workflow (`stable` branch) | 1 |
| 3 | Builder files on `ref: stable`, CI keeps them regenerated and compiles them | 2 |
| 4a | Add-on "Levoit ESPHome": device list, connect / create / manage configs, onboarding with the existing keys and secrets, recompile and upload | 2, 3 |
| 4b | Integration "Levoit ESPHome" (HACS): update entities, install through the add-on | 4a |
| 5 | Backup and restore page | — |
| 6 | DNS add-on (research first) | 3 |
| 7 | Back to stock: restore on the page, slot switch for DNS installs | 5, 6 |
| 8 | `.bin` upload in the add-on, at the user's own risk | 4a |

Phases 2 and 3 can ship before the add-on: builder files pasted into the
Device Builder on `ref: stable` get releases with their next install.

## Decisions

* **One version for the repo.** `levoit` and `philips` report the same
  release number from `components/levoit/VERSION`
* **An add-on plus a small integration.** The add-on holds the device page and
  does the work; the integration adds Home Assistant update entities
* **Configs go through the Device Builder API**, never through the config
  folder
* **Onboarding keeps the device's own keys and secrets** (API key, OTA
  password, name, Wi-Fi); fresh keys only for new devices
* **Both keep the name "Levoit ESPHome"**, although they also cover the
  Philips devices
* **`.bin` upload comes later** (phase 8), at the user's own risk, with the
  limits stated on the page
