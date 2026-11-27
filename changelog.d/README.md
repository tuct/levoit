# Change log fragments

One file per pull request, so two open PRs never conflict on
[CHANGELOG.md](../CHANGELOG.md). At the next release the maintainer runs
`python tools/changelog.py release`, which lays the files out under one
heading per scope, folds the ones with the same kind and area into one bullet
each, and moves them into the log. The log is read by change, not by PR.

**File name**: `<pr>-<slug>.md`, for example `151-core300s-sleep-speed.md`.
Open the PR first if you need the number; the PR link is appended from it.
A file without a number (`core300s-sleep-speed.md`) is accepted too, the link
is then yours to add.

**First line**: where the change lands and what kind it is, each in brackets,
then the *area*, the bold lead of the bullet: the device or feature the change
is about. Word the area so that the next PR on the same thing can reuse it
exactly; copy it from an existing fragment when one fits.

Where (one tag; a change that touches more than one family takes `[levoit]`):

| Tag | Heading | Covers |
|-----|---------|--------|
| `[project]` | Project | repo-wide: CI, change log tooling, builder files, docs structure |
| `[levoit]` | Levoit component | what every model shares, and changes that touch more than one family |
| `[levoit-core]` | Levoit Core models | Core 200S / 300S / 400S / 600S |
| `[levoit-vital]` | Levoit Vital models | Vital 100S / 200S, Everest Air, Sprout, Eleva 300X |
| `[levoit-humidifiers]` | Levoit humidifiers | Superior 6000S, the humidifier plan, the LV600S once it is a `model:` |
| `[lv600s]` | LV600S component | the separate `lv600s_humidifier` component |
| `[philips]` | Philips component | 600 and 900 series |

What (one tag): `[new]` (a model, a guide, a capture, a feature that was not
there), `[enhancement]` (a supported model does or reports more), `[fix]`
(something was wrong and is right now).

**The rest**: one sentence, two at most, on what changed for the user, for
which models, and why, plus the capture or issue it rests on. No bullet
marker, no trailing period, wrapped as you like. Links are relative to the
repo root, where CHANGELOG.md lives. Test reports and byte-level detail belong
in the PR; the check refuses an item past 300 characters.

```
[levoit-core][fix] Core 300S on the `01 32 40` status
speed reads the running speed in Sleep, `00` shown as the lowest speed, instead
of the last manual speed. Frames in
[`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)
```

Two fragments with that kind and area become one bullet, items in PR order,
each with its PR link, under the scope's heading; within a heading the bullets
run new, enhancement, fix:

```markdown
**Levoit Core models**

* **Fix** · **Core 300S on the `01 32 40` status**: Display reads off in Sleep
  and while off, as the panel does ([#148](https://github.com/tuct/levoit/pull/148));
  speed reads the running speed in Sleep, ... ([#151](https://github.com/tuct/levoit/pull/151))
```

If the component's behaviour changed, add a second fragment under
[`components/levoit/changelog.d/`](../components/levoit/changelog.d/) for the
component's own change log.

Check: `python tools/changelog.py check`. Preview: `python tools/changelog.py preview`.
