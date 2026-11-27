#!/usr/bin/env python3
"""Change log fragments: one file per pull request, assembled at release.

Two pull requests that both add a bullet at the top of the "Unreleased"
section conflict on the same lines every time. So a PR does not edit the
shared section; it adds a small file, and the release moves the files into
the logs:

    changelog.d/<pr>-<slug>.md                   -> CHANGELOG.md
    components/levoit/changelog.d/<pr>-<slug>.md -> components/levoit/README.md
                                                    (the component change log)

The log is summarised by change, not by PR. A fragment's first line names
the group in brackets (core, devices, enhancements, fixes) and the area (the
bold lead of the bullet), the rest is one sentence, two at most, on what
changed; fragments with the same area become one bullet at release, and the
bullets are laid out by group:

    [devices] Core 300S on the `01 32 40` status
    speed reads the running speed in Sleep, `00` shown as the lowest speed,
    instead of the last manual speed. Frames in
    [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)

    **Devices**

    * **Core 300S on the `01 32 40` status**: Display reads off in Sleep and
      while off, as the panel does ([#148](...)); speed reads the running
      speed in Sleep, ... ([#151](...))

The group is required in changelog.d/ and optional in the component log.

Links are relative to the log the fragment lands in. The PR link is
appended from the file name when the text does not mention #151 already.

    python tools/changelog.py check      validate fragments and the logs (CI)
    python tools/changelog.py preview    print the sections as they will appear
    python tools/changelog.py release [--date YYYY.MM.DD]
                                         move the fragments into the logs

`release` is for the release PR: bump components/levoit/VERSION first when
the component changed, run it, write the lead paragraph where it left a
"release lead" comment, commit.
"""

from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import sys
import textwrap
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PR_URL = "https://github.com/tuct/levoit/pull/{n}"
VERSION_FILE = REPO / "components" / "levoit" / "VERSION"

UNRELEASED_HEADING = "### Unreleased"
UNRELEASED_BODY = """\
Changes merged since the last release are collected in
[`changelog.d/`](./changelog.d/), one file per pull request, and land here at
the next release. `python tools/changelog.py preview` shows them as they will
appear."""

# Characters of item text, links counted by their text only. One sentence,
# two at most; a test report or a byte-by-byte account belongs in the PR.
MAX_ITEM = 300
WIDTH = 80

# The groups a release entry is laid out in, in this order. A project fragment
# names its group in brackets at the start of the area line ("[fixes] UART
# frame handling"); a component fragment may.
GROUPS = {
    "core": "Core",
    "devices": "Devices",
    "enhancements": "Enhancements",
    "fixes": "Fixes",
}

LEAD_MARK = "<!-- release lead:"
LEAD_COMMENT = (
    "<!-- release lead: one or two sentences on what this release is about, "
    "then delete this comment -->"
)


@dataclass
class Log:
    name: str
    fragments: Path
    target: Path


PROJECT = Log("project change log", REPO / "changelog.d", REPO / "CHANGELOG.md")
COMPONENT = Log(
    "component change log",
    REPO / "components" / "levoit" / "changelog.d",
    REPO / "components" / "levoit" / "README.md",
)


@dataclass
class Fragment:
    path: Path
    number: int | None
    area: str
    text: str
    group: str | None = None  # a key of GROUPS


def rel(path: Path) -> str:
    return path.relative_to(REPO).as_posix()


def read(path: Path) -> tuple[str, str]:
    """Return the text with \\n line ends, and the file's own line ending."""
    with path.open(encoding="utf-8", newline="") as fh:
        raw = fh.read()
    eol = "\r\n" if "\r\n" in raw else "\n"
    return raw.replace("\r\n", "\n"), eol


def write(path: Path, text: str, eol: str) -> None:
    with path.open("w", encoding="utf-8", newline="") as fh:
        fh.write(text.replace("\n", eol))


def parse_fragment(path: Path, errors: list[str], require_group: bool) -> Fragment | None:
    text, _ = read(path)
    text = text.strip()
    where = rel(path)
    if not text:
        errors.append(f"{where}: empty")
        return None
    lines = [ln.strip() for ln in text.split("\n")]
    area = lines[0]
    if area.startswith(("* ", "- ", "#")):
        errors.append(f"{where}:1: the first line is the area, the bold lead of the "
                      f"bullet, without '* ' or '#' (see {rel(PROJECT.fragments)}/README.md)")
        return None
    group = None
    m = re.match(r"^\[([^\]]*)\]\s*(.*)$", area)
    if m:
        tag, area = m.group(1).strip().casefold(), m.group(2).strip()
        if tag not in GROUPS:
            errors.append(f"{where}:1: unknown group [{tag}]; one of "
                          + ", ".join(f"[{g}]" for g in GROUPS))
            return None
        group = tag
    elif require_group:
        errors.append(f"{where}:1: start the area line with the group, one of "
                      + ", ".join(f"[{g}]" for g in GROUPS)
                      + f" (see {rel(PROJECT.fragments)}/README.md)")
        return None
    if area.startswith("**"):
        area = area.strip("*").rstrip(":").strip()
    if not area:
        errors.append(f"{where}:1: no area after the group")
        return None
    body = " ".join(ln for ln in lines[1:] if ln).strip()
    if body.endswith("."):
        body = body[:-1]
    if not body:
        errors.append(f"{where}: no text after the area line")
        return None
    visible = re.sub(r"\]\([^)]*\)", "]", body)
    if len(visible) > MAX_ITEM:
        errors.append(f"{where}: {len(visible)} characters; one sentence, two at most "
                      f"({MAX_ITEM} max), details belong in the PR")
        return None
    m = re.match(r"^(\d+)-", path.name)
    number = int(m.group(1)) if m else None
    if number is not None and not re.search(rf"#{number}\b", body):
        body += f" ([#{number}]({PR_URL.format(n=number)}))"
    return Fragment(path, number, area, body, group)


def load(log: Log, errors: list[str]) -> list[Fragment]:
    if not log.fragments.is_dir():
        return []
    found = []
    for path in sorted(log.fragments.iterdir()):
        if path.name == "README.md" or path.name.startswith("."):
            continue
        if path.suffix != ".md":
            errors.append(f"{rel(path)}: fragments are .md files")
            continue
        fragment = parse_fragment(path, errors, require_group=log is PROJECT)
        if fragment:
            found.append(fragment)
    # Newest PR first, like the logs; unnumbered files after them, by name.
    found.sort(key=lambda f: (f.number is None, -(f.number or 0), f.path.name))
    return found


def unreleased_span(text: str) -> tuple[int, int]:
    """Start and end offsets of the Unreleased section body in CHANGELOG.md."""
    start = text.find(UNRELEASED_HEADING + "\n")
    if start < 0:
        raise SystemExit(f"{rel(PROJECT.target)}: no '{UNRELEASED_HEADING}' heading")
    body = start + len(UNRELEASED_HEADING) + 1
    m = re.compile(r"^### ", re.M).search(text, body)
    return body, (m.start() if m else len(text))


def check_logs(errors: list[str]) -> None:
    text, _ = read(PROJECT.target)
    body_start, body_end = unreleased_span(text)
    if text[body_start:body_end].strip() != UNRELEASED_BODY:
        errors.append(
            f"{rel(PROJECT.target)}: the Unreleased section was edited. Describe the "
            f"change in a file under {rel(PROJECT.fragments)}/ instead "
            f"(see {rel(PROJECT.fragments)}/README.md)"
        )
    if LEAD_MARK in text:
        errors.append(f"{rel(PROJECT.target)}: write the release lead where the "
                      f"'release lead' comment is, then delete the comment")
    readme, _ = read(COMPONENT.target)
    if re.search(r"^#{2,6} .*unreleased", readme, re.I | re.M):
        errors.append(
            f"{rel(COMPONENT.target)}: no 'Unreleased' heading in the component change "
            f"log. Describe the change in a file under {rel(COMPONENT.fragments)}/"
        )
    if LEAD_MARK in readme:
        errors.append(f"{rel(COMPONENT.target)}: write the release lead where the "
                      f"'release lead' comment is, then delete the comment")


def report(errors: list[str]) -> int:
    for e in errors:
        if os.environ.get("GITHUB_ACTIONS"):
            where, _, rest = e.partition(": ")
            print(f"::error file={where.split(':')[0]}::{rest}")
        print(f"error: {e}", file=sys.stderr)
    return 1 if errors else 0


def bullets(fragments: list[Fragment]) -> str:
    """One bullet per area, newest area first, its items oldest first."""
    areas: dict[str, list[Fragment]] = {}
    for f in fragments:  # sorted newest first, so the dict keeps that order
        areas.setdefault(f.area.casefold(), []).append(f)
    out = []
    for items in areas.values():
        items.sort(key=lambda f: (f.number is None, f.number or 0, f.path.name))
        line = f"* **{items[0].area}**: " + "; ".join(f.text for f in items)
        out.append("\n".join(textwrap.wrap(
            line, WIDTH, subsequent_indent="  ",
            break_long_words=False, break_on_hyphens=False)))
    return "\n".join(out)


def assembled(fragments: list[Fragment]) -> str:
    """The bullets, laid out by group in GROUPS order under a bold group name.
    Fragments without a group (allowed in the component log) come last, with
    no heading; a log with no groups at all is a plain list, as before."""
    if not any(f.group for f in fragments):
        return bullets(fragments)
    parts = []
    for key, title in GROUPS.items():
        items = [f for f in fragments if f.group == key]
        if items:
            parts.append(f"**{title}**\n\n{bullets(items)}")
    rest = [f for f in fragments if f.group is None]
    if rest:
        parts.append(bullets(rest))
    return "\n\n".join(parts)


def cmd_check(_: argparse.Namespace) -> int:
    errors: list[str] = []
    for log in (PROJECT, COMPONENT):
        n = len(load(log, errors))
        plural = "s" if n != 1 else ""
        print(f"{log.name}: {n} fragment{plural} in {rel(log.fragments)}/")
    check_logs(errors)
    code = report(errors)
    print("OK" if code == 0 else f"{len(errors)} problem(s)")
    return code


def cmd_preview(_: argparse.Namespace) -> int:
    errors: list[str] = []
    project = load(PROJECT, errors)
    component = load(COMPONENT, errors)
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    print(f"{UNRELEASED_HEADING} (CHANGELOG.md)\n")
    print(assembled(project) if project else "_No fragments yet._")
    print(f"\n#### ESP Version: {version} - unreleased (components/levoit/README.md)\n")
    print(assembled(component) if component else "_No fragments yet._")
    return report(errors)


def newest_component_version(readme: str) -> str | None:
    m = re.search(r"^#### ESP Version: (\d+\.\d+\.\d+)", readme, re.M)
    return m.group(1) if m else None


def stray_bullets(body: str) -> str:
    """Bullets that ended up in the Unreleased section anyway."""
    keep = [ln for ln in body.split("\n") if ln.startswith("* ") or ln.startswith("  ")]
    return "\n".join(keep).strip()


def cmd_release(args: argparse.Namespace) -> int:
    errors: list[str] = []
    project = load(PROJECT, errors)
    component = load(COMPONENT, errors)
    if report(errors):
        return 1
    text, eol = read(PROJECT.target)
    body_start, body_end = unreleased_span(text)
    stray = stray_bullets(text[body_start:body_end])
    if not project and not component and not stray:
        print("nothing to release: no fragments")
        return 1
    date = args.date or dt.date.today().strftime("%Y.%m.%d")
    if not re.fullmatch(r"\d{4}\.\d{2}\.\d{2}", date):
        raise SystemExit(f"--date must be YYYY.MM.DD, got {date!r}")
    version = VERSION_FILE.read_text(encoding="utf-8").strip()

    heading = f"### {date}"
    bullets: list[str] = []
    if component:
        readme, readme_eol = read(COMPONENT.target)
        if newest_component_version(readme) == version:
            raise SystemExit(
                f"{rel(COMPONENT.fragments)}/ has fragments but {rel(VERSION_FILE)} is "
                f"still {version}, the newest entry in the component change log. Bump "
                f"VERSION first; the Version check wants both in one PR."
            )
        anchor = "## Change Log\n\n"
        at = readme.find(anchor)
        if at < 0:
            raise SystemExit(f"{rel(COMPONENT.target)}: no '## Change Log' heading")
        at += len(anchor)
        entry = (f"#### ESP Version: {version} - {date}\n\n{LEAD_COMMENT}\n\n"
                 f"{assembled(component)}\n\n")
        write(COMPONENT.target, readme[:at] + entry + readme[at:], readme_eol)
        heading += f" (component {version})"
        bullets.append(
            f"* **Component [{version}](./components/levoit/README.md#change-log)**: "
            f"{LEAD_COMMENT}"
        )
    bullets.append(assembled(project))
    if stray:
        bullets.append(stray)
    section = (f"\n{UNRELEASED_BODY}\n\n{heading}\n\n"
               + "\n".join(b for b in bullets if b) + "\n\n")
    write(PROJECT.target, text[:body_start] + section + text[body_end:], eol)

    for f in project + component:
        f.path.unlink()
    print(f"{rel(PROJECT.target)}: {heading}")
    if component:
        print(f"{rel(COMPONENT.target)}: #### ESP Version: {version} - {date}")
    print(f"removed {len(project) + len(component)} fragment(s)")
    print("Next: write the lead paragraph(s) at the 'release lead' comments, "
          "run 'python tools/changelog.py check', commit.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("check", help="validate fragments and the logs").set_defaults(run=cmd_check)
    sub.add_parser("preview", help="print the assembled sections").set_defaults(run=cmd_preview)
    release = sub.add_parser("release", help="move the fragments into the logs")
    release.add_argument("--date", help="release date as YYYY.MM.DD (default: today)")
    release.set_defaults(run=cmd_release)
    args = parser.parse_args(argv)
    return args.run(args)


if __name__ == "__main__":
    sys.exit(main())
