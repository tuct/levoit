# Working in this repository

The short list for coding assistants, and for anyone new here. The long
version is [CONTRIBUTING.md](./CONTRIBUTING.md); where the two differ,
CONTRIBUTING wins.

## What this is

ESPHome firmware for Levoit air purifiers and humidifiers, plus Philips
purifiers. The custom components replace the stock Wi-Fi module's firmware and
talk to the device's own MCU over UART, so every decoded byte traces back to a
captured frame from a real unit.

| Path | What |
|---|---|
| `components/levoit/` | The Levoit component. `levoit.cpp` owns the UART receive loop and the ESPHome glue, `decoder.cpp` dispatches frames, `*_status.cpp` / `*_commands.cpp` decode and build frames per model family (core, vital, sprout, superior) |
| `components/philips/` | The Philips component |
| `devices/<model>/` | Per-device YAML, wiring notes, README, and the captures (`uart/`) the decode came from |
| `LEVOIT_UART.md` | Protocol reference: frame layout, commands, status bytes |
| `docs/` | Capturing UART, adding a device, firmware updates |
| `tools/`, `.github/scripts/` | Maintainer and CI scripts |
| `projects/` | Earlier per-device projects, superseded by `devices/` |

## Before changing code

- **Captures decide.** A byte offset is right because a file under
  `devices/<model>/uart/` shows it, not because it looks plausible. Read
  `LEVOIT_UART.md` and the capture first, and cite both (file, which frames)
  in the PR and in the change log fragment.
- **One component, many MCU firmwares.** A fix for one model or firmware must
  not change what another does; the same model can have two status layouts
  (Core 300S: the stock one and `01 32 40`). Guard by model, keep the existing
  behaviour for everyone else, and say which models you changed.
- **Two ESPHome versions must build**: the minimum in the component READMEs
  (2026.6.0 today) and the newest release. Branch on `ESPHOME_VERSION_CODE`
  for API differences; never raise the minimum in a fix.
- **Match the surrounding style** and do not reformat. The C++ puts braces on
  their own lines with four-space indents.
- **Hands off**: `components/levoit/VERSION` and released change log entries
  (release PRs only); `devices/*/*-builder*.yaml` (generated, run
  `devices/make-builder-yaml.ps1` instead); `secrets.yaml` and anything with
  credentials or photo EXIF.

## Change log: one file per PR

Do not edit the *Unreleased* section of `CHANGELOG.md`, and do not add an
*Unreleased* heading to `components/levoit/README.md`; CI rejects both.
Instead add

- `changelog.d/<pr>-<slug>.md` for every user-visible change, and
- `components/levoit/changelog.d/<pr>-<slug>.md` as well when the component's
  behaviour changed.

A fragment is two parts: the first line names the group in brackets, one of
`[core]`, `[devices]`, `[humidifiers]`, `[fixes]`, and the area (the device or
feature; reuse an existing fragment's area word for word when one fits, so the
release folds them into one bullet), the rest is one sentence, two at most, on
what changed, for which models, and why, with the capture or issue it rests
on. No bullet marker, no test report, under 300 characters; the PR link is
appended from the file name. The release lays the entry out by group, so a
fragment is the whole summary of its PR: write it crisp. Format and example:
[`changelog.d/README.md`](./changelog.d/README.md). Run
`python tools/changelog.py check`.

## Verify, then say exactly what you verified

- Host tests (the protocol core built with a plain `g++`, no ESPHome; the
  commands are in `.github/workflows/host-tests.yml`): the frame layer, the
  device state, and `tests/host/levoit_device_test.cpp`, which drives the real
  `LevoitDevice` and decoders with captured frames, corrupted streams and
  commands through a fake platform and host. The same test under ASan and
  UBSan: `python components/levoit/tests/test_uart_integrity.py`
- Compile tests on both ESPHome versions: `components/run-compile-tests.sh min`,
  or on Windows `.\components\run-compile-tests.ps1 min` in PowerShell (not Git
  Bash, where the ESP-IDF build is skipped while ESPHome still reports success)
- Change log: `python tools/changelog.py check`
- You cannot run the firmware on a purifier. Write "compiles, not tested on
  hardware" and stop there. Do not describe device behaviour as verified, and
  do not restate a person's test report as more than it says.
- Automated review comments on the PR get an answer, each one: a fix, or why
  the finding is wrong.

## The pull request

- Branch `type/short-description` off `main`, PR against `main`, one concern
  per PR. Commits follow Conventional Commits with the scopes in the history:
  `fix(levoit): ...`, `feat(philips): ...`, `docs(...)`, `test(...)`,
  `chore(...)`; the body says why.
- Fill in the PR template: what and why, models affected, how it was
  verified, the fragment. Plain and short. No summary adjectives
  ("comprehensive", "robust"), no validation claims the PR does not contain.
