"""Check devices/onboarding.json, the content of Add device in Levoit ESPHome.

    python .github/scripts/check_onboarding.py

The integration (tuct/levoit-esphome-ha) reads the file from main: per device
a photo, a status and a few words, shown when a device is added. This checks
that it parses, has only the known fields and values, that every photo and
guide exists in the repo, and that every device with a template in
devices/templates.json has an entry. Prints what is wrong; exit 1 if anything.
"""

from __future__ import annotations

import json
from pathlib import Path
import re
import sys

REPO = Path(__file__).resolve().parents[2]
FILE = REPO / "devices" / "onboarding.json"
TEMPLATES = REPO / "devices" / "templates.json"

REQUIRED = {"id", "title", "photo", "guide", "status", "summary", "open", "routes"}
OPTIONAL = {"status_note", "spec"}
STATUS = {"tested", "partly_tested", "untested", "development", "planned", "custom"}
OPEN = {"easy", "medium", "hard"}
ROUTES = {"flash", "add_esp", "custom"}
ID = re.compile(r"^[a-z0-9][a-z0-9-]*$")
# a path in the repo, no way out of it
PATH = re.compile(r"^(?!/)(?!.*\.\.)[A-Za-z0-9_./-]+$")
MAX_TEXT = 120


def problems() -> list[str]:
    errors: list[str] = []
    try:
        data = json.loads(FILE.read_text(encoding="utf-8"))
    except (OSError, ValueError) as err:
        return [f"{FILE.relative_to(REPO)}: {err}"]
    if not isinstance(data, dict) or data.get("version") != 1 or set(data) != {"version", "devices"}:
        return ['the file is {"version": 1, "devices": [...]}']
    devices = data["devices"]
    if not isinstance(devices, list):
        return ["devices is a list"]

    seen: set[str] = set()
    for index, entry in enumerate(devices):
        where = f"devices[{index}]"
        if not isinstance(entry, dict):
            errors.append(f"{where}: not an object")
            continue
        where = f"{where} {entry.get('id', '?')}"
        if missing := REQUIRED - entry.keys():
            errors.append(f"{where}: missing {', '.join(sorted(missing))}")
        if unknown := entry.keys() - REQUIRED - OPTIONAL:
            errors.append(f"{where}: unknown {', '.join(sorted(unknown))}")
        for key in ("id", "title", "photo", "guide", "status", "summary", "open", *OPTIONAL):
            if key in entry and (not isinstance(entry[key], str) or not entry[key].strip()):
                errors.append(f"{where}: {key} is a non-empty string")
        for key in ("title", "summary", "status_note", "spec"):
            if isinstance(entry.get(key), str) and len(entry[key]) > MAX_TEXT:
                errors.append(f"{where}: {key} is longer than {MAX_TEXT} characters")
        ident = entry.get("id")
        if isinstance(ident, str):
            if not ID.match(ident):
                errors.append(f"{where}: id is lowercase letters, digits and hyphens")
            if ident in seen:
                errors.append(f"{where}: id used twice")
            seen.add(ident)
        for key in ("photo", "guide"):
            path = entry.get(key)
            if isinstance(path, str):
                if not PATH.match(path):
                    errors.append(f"{where}: {key} is a path in the repo")
                elif not (REPO / path).exists():
                    errors.append(f"{where}: {key} {path} does not exist")
        if isinstance(entry.get("photo"), str) and not entry["photo"].lower().endswith((".jpg", ".jpeg", ".png", ".webp")):
            errors.append(f"{where}: photo is a .jpg, .png or .webp")
        if entry.get("status") not in STATUS | {None}:
            errors.append(f"{where}: status is one of {', '.join(sorted(STATUS))}")
        if entry.get("open") not in OPEN | {None}:
            errors.append(f"{where}: open is one of {', '.join(sorted(OPEN))}")
        routes = entry.get("routes")
        if routes is not None and (
            not isinstance(routes, list) or not routes or not set(routes) <= ROUTES or len(set(routes)) != len(routes)
        ):
            errors.append(f"{where}: routes is a list of {', '.join(sorted(ROUTES))}")

    templates = json.loads(TEMPLATES.read_text(encoding="utf-8")).get("templates", [])
    folders = sorted({t["file"].split("/")[1] for t in templates})
    for folder in folders:
        if folder not in seen:
            errors.append(f"{folder} has a template but no entry")
    return errors


def main() -> int:
    errors = problems()
    for error in errors:
        print(f"devices/onboarding.json: {error}")
    if not errors:
        print("devices/onboarding.json: ok")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
