[← Back to the project README](./README.md)

# Contributing

Contributions are welcome — a new device, a fix, a firmware dump, or a
correction to the docs. This page is about the **GitHub side**: issues, pull
requests and CI. For the hardware work, see
[Adding a new device](./docs/adding-a-new-device.md).

## Issues

Useful things to open an issue for:

- **A device that isn't supported yet.** Include the model number, photos of the
  PCB, and any chip markings. See
  [Adding a new device](./docs/adding-a-new-device.md) for what helps most.
- **A supported device behaving wrongly.** Include the model, the component
  version (the `esp_version` entity, or `get_version()` in the component), the
  MCU firmware version (`mcu_version`), and a log with `logger: level: DEBUG`.
  `VERBOSE` additionally logs raw UART frames, which is usually what is needed.
- **A newer MCU firmware** on a device that already works. A UART dump is the
  most useful thing you can attach.

Attach dumps as files rather than pasting them inline — they are long, and the
raw bytes matter.

## Pull requests

- **Branch from `main`** and open the PR against `main`.
- **One concern per PR** where you can. A device folder, a component fix and a
  docs restructure are three PRs, not one — they get reviewed and reverted
  independently.
- **Branch names** follow `type/short-description`, e.g.
  `fix/core200s-filter-life`, `feat/levoit-superior-6000s`,
  `docs/split-readme`.
- **Commit messages** use the [Conventional Commits](https://www.conventionalcommits.org/)
  prefixes already in the history — `feat(levoit):`, `fix(philips):`,
  `docs(...)`, `test(...)`, `chore(...)`. Explain *why* in the body, not just
  what; the diff already says what.

### What a good PR includes

| Change | Also include |
|--------|--------------|
| New device | `devices/<model>/` with example YAML, wiring notes and a README; the UART dumps you decoded it from; a row in the README table |
| Component behaviour | Which model(s) it affects, and how it was verified — hardware, or a compile test |
| Protocol decode | The capture backing it, committed under the device's folder |
| Anything user-visible | A [CHANGELOG.md](./CHANGELOG.md) entry, and the component's own change log if it is a component change |

State plainly what you **did** and **did not** verify. "Compiles, not tested on
hardware" is a perfectly good PR — silently implying otherwise is not.

## CI

[`.github/workflows/compile-tests.yml`](.github/workflows/compile-tests.yml)
runs on every PR that touches `components/`. It builds these configs from
[`components/levoit/tests`](./components/levoit/tests) and
[`components/philips/tests`](./components/philips/tests) (the Ethernet config
exists for levoit only):

| Config | Covers |
|--------|--------|
| `test-all-entities.yaml` | Every entity type on every platform |
| `test-fan-only.yaml` | Only a fan configured — catches unguarded platform includes |
| `test-foreign-platforms.yaml` | Other components supplying the platforms — catches guards on the generic `USE_*` macros |
| `test-ethernet.yaml` | Ethernet instead of Wi-Fi — catches code that depends on the `wifi` component (levoit only) |

Run them locally before pushing; they need no `secrets.yaml`. One script per
platform builds every `components/<component>/tests/test-*.yaml` and prints a
PASS/FAIL summary (exit code = number of failures). A new test config is picked
up automatically; in the workflow, one that exists for only one component goes
in the matrix `include`, once per ESPHome version (see the comment there).

| | Newest / your installed ESPHome | Minimum supported ESPHome |
|---|---|---|
| macOS / Linux | `components/run-compile-tests.sh` | `components/run-compile-tests.sh min` |
| Windows (PowerShell) | `.\components\run-compile-tests.ps1` | `.\components\run-compile-tests.ps1 min` |

- `min` reads the pinned version from the workflow and installs it once into
  `.venv-esphome-<version>/` at the repo root (git-ignored); your normal ESPHome
  install is not touched. Any other version works the same way, e.g. `2026.10.0`.
- Limit to one component: `run-compile-tests.sh min levoit` /
  `run-compile-tests.ps1 min -Component levoit`.
- Full build logs land in `components/<component>/tests/.esphome/<config>.log`.
- When the ESPHome version differs from the last run, the scripts delete the
  generated `components/<component>/tests/.esphome/build` first: files left by
  another version break the link (e.g. `undefined reference to
  __wrap___cxa_atexit` going from 2026.11 back to 2026.5.3), and
  `esphome clean` does not remove them.
- Windows: use PowerShell, not Git Bash — under MSYS the ESP-IDF build is
  skipped while ESPHome still reports success. If scripts are blocked:
  `powershell -ExecutionPolicy Bypass -File components\run-compile-tests.ps1 min`.

A single config by hand:

```bash
cd components/levoit/tests
esphome compile test-all-entities.yaml
```

### ESPHome versions

CI builds every config twice: with the **newest** ESPHome (unpinned, so a new
ESPHome release can turn the checks red without any change in this repo) and
with the **minimum** stated in the component READMEs (currently **2026.6.0**,
the `esphome` matrix in the workflow; raise both together). Reproduce the
minimum build locally with the `min` argument of the scripts above.

If an API changed between the two, keep both working with a version check
rather than raising the minimum:

```cpp
#include "esphome/core/version.h"
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 11, 0)
  // new API
#else
  // old API
#endif
```

**If you add an entity type**, add it to `test-all-entities.yaml` in the same
PR, or CI will not cover it.

Device configs under `devices/` are *not* built by CI — they need a
`secrets.yaml`. Build those locally with `devices/build-all-dev.ps1`.

The **builder files** (`devices/*/*-builder*.yaml`) are what users paste into
the ESPHome Device Builder. They are generated: after changing a device yaml
or its `common.yaml`, run `devices/make-builder-yaml.ps1` and commit the
result. The `Builder files` workflow fails when one is out of date, and
compiles them against the components of the PR (with dummy secrets): a PR
that only touches `devices/<x>/` builds `<x>`'s builder files, one that
touches `components/` or `.github/` builds all of them, in jobs of up to five
files (`.github/scripts/builder_ci_matrix.py`). Pushes to `main` build all.
A key such as `logger:` must be set in only one of the device yaml
and `common.yaml`: ESPHome merges packages, a single inlined file cannot.

Builder files pull the component from the `stable` branch (the latest
release), while CI builds them against the PR. So a device yaml or
`common.yaml` may use a new component option only once that option is in a
release; until then keep it in the `_dev` configs.

## Versioning

The repo has one version, for both components, in one place:
[`components/levoit/VERSION`](./components/levoit/VERSION). The build compiles
it in (`LEVOIT_VERSION`, `PHILIPS_VERSION`) together with the git commit it was
built from, and the `esp_version` sensors report it (`1.7.0+3fa21c4 esphome`;
see [`version_info.py`](./components/levoit/version_info.py)). It must
match the newest released entry (`#### ESP Version: X.Y.Z - date`) in the
[change log](./components/levoit/README.md#change-log). Bump both in the same
PR, or neither — a changelog entry for a version the binary does not report is
worse than no entry. The `Version check` workflow enforces this on every PR
that touches the component, and checks that a release tag matches the file.

Publishing a release on GitHub (tag `X.Y.Z`, notes written by hand) runs the
`Release` workflow, which moves the `stable` branch forward to that tag.
Configs that pull the components with `ref: stable` get releases only; `main`
gets every merge.

## Secrets

`secrets.yaml` is gitignored everywhere and must stay that way. Never commit
credentials, and check that photos and captures do not carry them either —
phone photos routinely include GPS EXIF, and device labels can show MAC
addresses.

## Reviews

PRs get an automated review, and it is worth reading: it has caught real bugs
here. Push a fix or explain why it is wrong — either is fine, but don't merge
over an unanswered finding.
