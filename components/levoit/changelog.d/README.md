# Component change log fragments

One file per pull request that changes what the `levoit` component does. At
the next component release the maintainer bumps [`VERSION`](../VERSION) and
runs `python tools/changelog.py release`, which moves these files into the
[change log](../README.md#change-log) under the new `#### ESP Version` heading.

Same rules as [`changelog.d/`](../../../changelog.d/README.md) at the repo
root: `<pr>-<slug>.md`; first line the area, then one sentence, two at most,
on what changed; fragments with the same area become one bullet, the PR link
is appended from the file name. The tags for where (`[levoit-core]` and the
others) and what (`[new]`, `[enhancement]`, `[fix]`) are optional here; with
them, the entry is laid out like the project log. Links are relative to
`components/levoit/README.md`, so a capture is
`../../devices/levoit-core300s/uart/status_32_40.txt`.

The lead paragraph of a release (what the release is about, who tested it) is
written by hand in the release PR, not in fragments.

A user-visible change gets a fragment in the root `changelog.d/` as well.
