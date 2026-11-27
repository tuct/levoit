# Change log fragments

One file per pull request, so two open PRs never conflict on
[CHANGELOG.md](../CHANGELOG.md). At the next release the maintainer moves the
files into the log with `python tools/changelog.py release`.

**File name**: `<pr>-<slug>.md`, for example `151-core300s-sleep-speed.md`.
Open the PR first if you need the number; the PR link is appended from it.
A file without a number (`core300s-sleep-speed.md`) is accepted too, the link
is then yours to add.

**Content**: the finished Markdown bullet, exactly as it will appear in the
log, as one tight list (no blank lines). Links are relative to the repo root,
where CHANGELOG.md lives.

```markdown
* **Core 300S on the `01 32 40` status: speed reads 1 in Sleep.** The fan showed
  the last manual speed in Sleep, often 3, though it runs slower than speed 1. It
  now reads the running speed, `00` in Sleep, as Auto already did. Frames in
  [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt) ("sleep")
```

A good entry says **what changed for the user, on which models, and why**, and
points at the capture or issue behind it. Write it for the person reading the
release notes, not for the reviewer.

If the component's behaviour changed, add a second fragment under
[`components/levoit/changelog.d/`](../components/levoit/changelog.d/) for the
component's own change log.

Check: `python tools/changelog.py check`. Preview: `python tools/changelog.py preview`.
