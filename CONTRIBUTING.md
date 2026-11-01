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
[`components/levoit/tests`](./components/levoit/tests):

| Config | Covers |
|--------|--------|
| `test-all-entities.yaml` | Every entity type on every platform |
| `test-fan-only.yaml` | Only a fan configured — catches unguarded platform includes |
| `test-foreign-platforms.yaml` | Other components supplying the platforms — catches guards on the generic `USE_*` macros |
| `test-ethernet.yaml` | Ethernet instead of Wi-Fi — catches code that depends on the `wifi` component (levoit only) |

Run them locally before pushing; they need no `secrets.yaml`:

```bash
cd components/levoit/tests
esphome compile test-all-entities.yaml
```

**If you add an entity type**, add it to `test-all-entities.yaml` in the same
PR, or CI will not cover it.

Device configs under `devices/` are *not* built by CI — they need a
`secrets.yaml`. Build those locally with `devices/build-all-dev.ps1`.

## Versioning

The levoit component version lives in `components/levoit/levoit.h`
(`get_version()`) and must match the newest entry in its
[change log](./components/levoit/README.md#change-log). Bump both in the same
PR, or neither — a changelog entry for a version the binary does not report is
worse than no entry.

## Secrets

`secrets.yaml` is gitignored everywhere and must stay that way. Never commit
credentials, and check that photos and captures do not carry them either —
phone photos routinely include GPS EXIF, and device labels can show MAC
addresses.

## Reviews

PRs get an automated review, and it is worth reading: it has caught real bugs
here. Push a fix or explain why it is wrong — either is fine, but don't merge
over an unanswered finding.
