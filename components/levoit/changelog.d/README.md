# Component change log fragments

One file per pull request that changes what the `levoit` component does. At
the next component release the maintainer bumps [`VERSION`](../VERSION) and
runs `python tools/changelog.py release`, which moves these files into the
[change log](../README.md#change-log) under the new `#### ESP Version` heading.

Same rules as [`changelog.d/`](../../../changelog.d/README.md) at the repo
root: `<pr>-<slug>.md`, the finished Markdown bullet as one tight list, the PR
link appended from the file name. Links are relative to
`components/levoit/README.md`, so a capture is
`../../devices/levoit-core300s/uart/status_32_40.txt`.

The lead paragraph of a release (what the release is about, who tested it) is
written by hand in the release PR, not in fragments.

A user-visible change gets a fragment in the root `changelog.d/` as well.
