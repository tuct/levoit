"""Which builder files a CI run compiles, in a few groups.

    git diff --name-only <base>...HEAD | python .github/scripts/builder_ci_matrix.py
    python .github/scripts/builder_ci_matrix.py --all

Prints a JSON list for the compile job's matrix, one entry per job:
{"name": "builds 1/2: levoit-core200s, levoit-core300s", "files": "devices/... devices/..."}

From the changed files (stdin):

* devices/<device>/...       -> the builder files of that device
* components/, .github/, other files in devices/ (the generator script)
                             -> every builder file
* everything else (docs/, READMEs, images, devices/templates.json,
  devices/onboarding.json)
                             -> nothing

The files are split into groups of at most GROUP_SIZE, built one after another
in one job: the ESP-IDF set-up then happens once per group, and a PR takes a
few job slots instead of one per file.
"""

from __future__ import annotations

import json
import math
from pathlib import Path
import sys

REPO = Path(__file__).resolve().parents[2]
DEVICES = REPO / "devices"
GROUP_SIZE = 5

# Changed paths that do not change any build.
NO_BUILD = ("devices/templates.json", "devices/onboarding.json")
NO_BUILD_SUFFIXES = (".md", ".png", ".jpg", ".jpeg", ".gif", ".bin", ".sal")
# Changed paths under these change every build.
BUILD_ALL = ("components/", ".github/", "devices/")


def all_builder_files() -> list[str]:
    return sorted(p.relative_to(REPO).as_posix() for p in DEVICES.glob("*/*-builder*.yaml"))


def affected(changed: list[str]) -> list[str]:
    """The builder files the changed paths can break."""
    everything = all_builder_files()
    folders: set[str] = set()
    for path in changed:
        if path in NO_BUILD or path.lower().endswith(NO_BUILD_SUFFIXES):
            continue
        parts = path.split("/")
        if len(parts) >= 3 and parts[0] == "devices":
            if (DEVICES / parts[1]).is_dir():
                folders.add(parts[1])
            # else: a deleted device folder, nothing left to build
        elif path.startswith(BUILD_ALL):
            return everything
    return [f for f in everything if f.split("/")[1] in folders]


def groups(files: list[str]) -> list[dict[str, str]]:
    """At most GROUP_SIZE files per group, as even as possible, in path order."""
    if not files:
        return []
    count = math.ceil(len(files) / GROUP_SIZE)
    size = math.ceil(len(files) / count)
    result = []
    for index in range(count):
        chunk = files[index * size : (index + 1) * size]
        devices = sorted({f.split("/")[1] for f in chunk})
        result.append(
            {
                "name": f"builds {index + 1}/{count}: {', '.join(devices)}",
                "files": " ".join(chunk),
            }
        )
    return result


def main() -> int:
    if "--all" in sys.argv[1:]:
        files = all_builder_files()
    else:
        changed = [line.strip() for line in sys.stdin if line.strip()]
        files = affected(changed)
    print(json.dumps(groups(files)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
