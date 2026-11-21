[← Back to the project README](../README.md)

# Firmware updates from Home Assistant — plan

> **Status: plan, nothing implemented yet.** This page describes the target design
> and the order it gets built in. Comment in an issue before starting on a part
> of it.

Today every user builds their own firmware from the device YAML (`ref: main` in
[`devices/packages/source_git.yaml`](../devices/packages/source_git.yaml)) and
finds out about new releases by watching this repo. The goal is to see and
install new versions from Home Assistant, the way ESPHome's own "ready-made
projects" do — for both ways this repo is used:

| Track | Hardware | Who builds the firmware | How it updates |
|-------|----------|-------------------------|----------------|
| **Prebuilt** | The device's **original ESP** (Core 200S/300S/400S/600S, Everest Air, Sprout, Superior 6000S, Vital 100S/200S, Eleva 300X …) | CI in this repo | The device itself, from a manifest on GitHub Pages |
| **Self-built** | An **additional ESP** (XIAO C3/S3/C6, Ethernet boards, own wiring and pins) | The user, in the ESPHome Device Builder | The ESPHome Device Builder, triggered from Home Assistant |

Only the original ESP gets prebuilt images: its pins are fixed by the board,
while additional ESPs are wired differently in almost every install.

## Goals

* Home Assistant shows **"update available"** with release notes for every
  device running the `levoit` component
* A **channel** per device: `stable` (tagged releases) or `main` (every merge)
* **One version number** everywhere: the release tag, the firmware, the
  `ESP Version` sensor and the Home Assistant integration
* Prebuilt images carry **no secrets**: Wi-Fi comes from the captive portal or
  Improv, the API encryption key is set by Home Assistant when the device is
  adopted
* Later: flash a **stock** device without opening it, through a DNS add-on that
  serves ESPHome in place of the VeSync OTA
* A way **back to the original VeSync firmware**

## Non-goals

* No custom update protocol. Everything on the device side is stock ESPHome
  (`http_request` update + OTA); the device updates fine without the Home
  Assistant integration installed
* No prebuilt images for additional ESPs

## Building blocks already in ESPHome

Checked against ESPHome 2026.11 and the minimum 2026.5.3:

* **`update:` platform `http_request`** reads a manifest
  (`name`, `version`, `builds[].chipFamily`, `builds[].ota.path` / `md5` /
  `summary` / `release_url`) and installs through `ota: platform: http_request`.
  An update is offered whenever the manifest `version` **differs** from the
  firmware's `esphome.project.version` — a plain string comparison, so
  switching from `main` back to `stable` (a downgrade) works too
* **`set_source_url()`** on that update entity exists in both versions, so a
  `select` can switch the manifest URL — the channel — at runtime
* **`api: encryption: {}`** without a key builds an image with no key in it;
  Home Assistant sets the key on adoption (`NoiseEncryptionSetKeyRequest`) and
  the device stores it in flash, where it survives later OTA updates
* **`dashboard_import`** lets the ESPHome Device Builder adopt a running device
  from its package URL

## Design

### 1. One version, from one place

A `VERSION` file at the repo root is the single source:

* the component reads it in `__init__.py` and defines it for C++; it replaces
  the hard-coded string in `get_version()` (`"1.7.0 esphome"` today)
* prebuilt configs pass it to `esphome.project.version`
* the integration's `manifest.json` carries the same version
* `main` builds are labelled `<next>-dev+<short sha>`, e.g. `1.8.0-dev+3fa21c4`
* CI fails if the tag, `VERSION` and `manifest.json` disagree

### 2. Release pipeline (prebuilt track)

A new workflow next to `compile-tests.yml`:

| Trigger | Channel | Version |
|---------|---------|---------|
| Tag `X.Y.Z` | `stable` | `X.Y.Z` |
| Push to `main` | `main` | `X.Y.Z-dev+<sha>` |

For every original-ESP device config it runs `esphome compile` and publishes to
GitHub Pages:

```
firmware/<device>/<channel>/manifest.json
firmware/<device>/<channel>/<device>.ota.bin
firmware/<device>/<channel>/<device>.factory.bin
```

`manifest.json` follows the ESP Web Tools format that ESPHome reads:

```json
{
  "name": "Levoit Core 300S",
  "version": "1.8.0",
  "home_assistant_domain": "esphome",
  "builds": [{
    "chipFamily": "ESP32",
    "ota": {
      "path": "levoit-core300s.ota.bin",
      "md5": "…",
      "summary": "First line of the release notes",
      "release_url": "https://github.com/tuct/levoit/releases/tag/1.8.0"
    },
    "parts": [{ "path": "levoit-core300s.factory.bin", "offset": 0 }]
  }]
}
```

`chipFamily` must match ESPHome's variant name (`ESP32` for the SOLO-1C units,
`ESP32-C3` for the ESP32-C3-SOLO-1 units).

### 3. Device package `devices/packages/updates.yaml`

Included by the prebuilt configs only:

```yaml
esphome:
  project:
    name: tuct.${device_name}
    version: ${levoit_version}

dashboard_import:
  package_import_url: github://tuct/levoit/devices/${device_name}/${device_name}.yaml@main

api:
  encryption: {}          # key set by Home Assistant on adoption

wifi:
  ap: {}                  # no credentials in the image
captive_portal:
improv_serial:

http_request:
ota:
  - platform: esphome
  - platform: http_request

update:
  - platform: http_request
    id: firmware_update
    name: Firmware
    source: https://tuct.github.io/levoit/firmware/${device_name}/stable/manifest.json

select:
  - platform: template
    name: Update channel
    options: [stable, main]
    restore_value: true
    optimistic: true
    entity_category: config
    on_value:
      - lambda: |-
          id(firmware_update).set_source_url(
            "https://tuct.github.io/levoit/firmware/${device_name}/" + x + "/manifest.json");
      - component.update: firmware_update
```

The sketch shows the shape, not final code (the boot-time source URL has to
follow the restored channel, for example).

### 4. Web installer

A page on the same GitHub Pages site using ESP Web Tools and the factory
images: first flash of an original ESP from the browser over USB, no ESPHome
install needed. After that, every update comes over the air.

### 5. Home Assistant integration "Levoit ESPHome" (HACS)

Lives in this repo at `custom_components/levoit_esphome/`, with a `hacs.json`
at the root. HACS requires the integration under `custom_components/` at the
repo root and allows one integration per repo; it ignores everything else
(`components/`, `devices/`). HACS takes its versions from this repo's GitHub
releases, so the integration shares the release number with the component.

What it does:

* **Discovery:** finds ESPHome devices running the `levoit` component through
  the ESPHome integration (project name `tuct.*`, or the `ESP Version` sensor)
* **One update entity per device:** installed version, latest version of the
  device's channel, release notes from the GitHub release
* **Channel:** shows and sets the device's `Update channel` select
* **Install:**
  * prebuilt: presses the device's own `Firmware` update entity
  * self-built: asks the ESPHome Device Builder add-on to compile and upload
    that device's config (the same path Home Assistant's ESPHome update entity
    uses when the add-on is installed)

For self-built configs, the channel is the git ref the config builds from. The
release workflow moves a `stable` branch to each new tag, so a config can use
`ref: stable` or `ref: main`; the integration compares the device's version
with the newest release or `main` commit and triggers a rebuild.

### 6. Later: DNS add-on for stock devices

A Home Assistant add-on that answers DNS for the VeSync firmware host and serves
an ESPHome image in its place, so a stock unit can be flashed without opening
it — the approach
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
| Serial / web installer with ESPHome's partition table | Overwritten | Write back the user's **own** flash backup |

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
* The web installer offers **"Back up first"** before the first ESPHome flash
  and stores the file locally; restoring is the same page with the backup file

For the DNS route, the ESPHome image has to keep the stock partition table
(two 1.5 MB slots) rather than bring its own, so the stock slot and `fac_data`
stay untouched.

### Help wanted: stock firmware URLs per model

Both the DNS route and "back to stock" need the stock firmware's download URL
for each model. The VeSync cloud only answers for devices that are in the
asking account — the request also has a `configModelList` parameter, but its
format is unknown (every shape tried is rejected as `illegal argument`) — so
this needs owners.

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
| 1 | `VERSION` file, component define, CI consistency check | — |
| 2 | Release workflow + GitHub Pages layout + manifests | 1 |
| 3 | `updates.yaml` package, prebuilt configs switched to it | 2 |
| 4 | Web installer page | 2 |
| 5 | HACS integration "Levoit ESPHome" | 3 |
| 6 | DNS add-on (research first) | 3 |
| 7 | Back to stock: backup / restore in the web installer, slot switch for DNS installs | 4, 6 |

Phases 1–3 make one PR; the device side then works without the integration.

## Open questions

* **OTA password on prebuilt images.** An image without secrets also has no
  `ota: platform: esphome` password, so anyone on the LAN could flash it. Drop
  native OTA from prebuilt images (updates only through `http_request` and
  Home Assistant), or accept it like ESPHome's ready-made projects do?
* **Adopted devices.** Once a user adopts a prebuilt device into the Device
  Builder and builds it themselves, the `http_request` update would overwrite
  their build. The adopted config should drop the update package; the
  integration then treats the device as self-built
* **Philips devices.** The `philips` component lives in the same repo — same
  integration, or out of scope?
