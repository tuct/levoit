# Change log fragments

One file per pull request, so two open PRs never conflict on
[CHANGELOG.md](../CHANGELOG.md). At the next release the maintainer runs
`python tools/changelog.py release`, which lays the files out by group, folds
the ones with the same area into one bullet each, and moves them into the
log. The log is read by change, not by PR.

**File name**: `<pr>-<slug>.md`, for example `151-core300s-sleep-speed.md`.
Open the PR first if you need the number; the PR link is appended from it.
A file without a number (`core300s-sleep-speed.md`) is accepted too, the link
is then yours to add.

**First line**: the group in brackets, then the *area*, the bold lead of the
bullet: the device or feature the change is about. Word the area so that the
next PR on the same thing can reuse it exactly; copy it from an existing
fragment when one fits.

| Group | For |
|-------|-----|
| `[core]` | the component's internals and tooling, what every model shares: protocol core, build, CI, change log |
| `[devices]` | one model or family: what it reports, what it can be told, its entities, its docs |
| `[humidifiers]` | the humidifier models and their plans |
| `[fixes]` | something that was wrong and is right now |

**The rest**: one sentence, two at most, on what changed for the user, for
which models, and why, plus the capture or issue it rests on. No bullet
marker, no trailing period, wrapped as you like. Links are relative to the
repo root, where CHANGELOG.md lives. Test reports and byte-level detail belong
in the PR; the check refuses an item past 300 characters.

```
[devices] Core 300S on the `01 32 40` status
speed reads the running speed in Sleep, `00` shown as the lowest speed, instead
of the last manual speed. Frames in
[`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)
```

Two fragments with that area become one bullet, items in PR order, each with
its PR link, under the group's heading:

```markdown
**Devices**

* **Core 300S on the `01 32 40` status**: Display reads off in Sleep and while
  off, as the panel does; the setting is kept and comes back on leaving Sleep
  or turning on ([#148](https://github.com/tuct/levoit/pull/148)); speed reads
  the running speed in Sleep, ... ([#151](https://github.com/tuct/levoit/pull/151))
```

If the component's behaviour changed, add a second fragment under
[`components/levoit/changelog.d/`](../components/levoit/changelog.d/) for the
component's own change log.

Check: `python tools/changelog.py check`. Preview: `python tools/changelog.py preview`.
