<!-- One concern per PR, branched from main, opened against main.
     CONTRIBUTING.md has the details; AGENTS.md is the short version. -->

## What and why

<!-- What changes for a user, and why. The diff says how. -->

## Models affected

<!-- e.g. "Core 300S with the 01 32 40 status (MCU 1.0.0)", "all Vital", "none, docs only" -->

## How it was verified

<!-- Say what you did and did not test. "Compiles, not tested on hardware" is a
     perfectly good answer; implying more than that is not.
     Hardware: model, MCU firmware (mcu_version entity), what you exercised.
     Compile: components/run-compile-tests.sh min  (or the .ps1), or CI only.
     Protocol: which capture under devices/<model>/uart/ backs the decode. -->

## Checklist

- [ ] Change log fragment `changelog.d/<pr>-<slug>.md` added, first line `[where][what] <area>` (e.g. `[levoit-core][fix]`, see `changelog.d/README.md`), one sentence after it (and `components/levoit/changelog.d/` for a component change), or no user-visible change
- [ ] A decode change cites a capture committed under `devices/<model>/uart/`
- [ ] Builder files regenerated with `devices/make-builder-yaml.ps1` if a device yaml or `common.yaml` changed
- [ ] `components/levoit/VERSION` and released change log entries untouched (release PRs only)
- [ ] If an AI assistant wrote part of this: said so above, and the verification section covers only what a person actually ran
