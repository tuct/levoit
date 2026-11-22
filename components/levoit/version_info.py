"""The repo version: components/levoit/VERSION plus the git commit it was built from.

One release number for the whole repo. levoit imports this module, and
components/philips/__init__.py loads it by path, so it must stay plain Python
with no esphome imports.

  1.7.0                 no git checkout around the component (copied folder, no git)
  1.7.0+3fa21c4         built from commit 3fa21c4 (external_components git clone,
                        or a local checkout of this repo)
  1.7.0+3fa21c4.dirty   local checkout with uncommitted changes under components/

The commit tells a build from main between two releases apart from the release
itself; the Home Assistant integration compares it with the head of the
device's channel. "+..." is semver build metadata, so 1.7.0+3fa21c4 still
sorts as 1.7.0.
"""

from pathlib import Path
import subprocess

_HERE = Path(__file__).resolve().parent
_REPO = _HERE.parent.parent  # <repo>/components/levoit -> <repo>


def _git(*args: str) -> str | None:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=_HERE,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    return result.stdout.strip() if result.returncode == 0 else None


def release() -> str:
    """The release number from VERSION, e.g. "1.7.0"."""
    return (_HERE / "VERSION").read_text(encoding="utf-8").strip()


def commit() -> str | None:
    """Short commit of this repo, with ".dirty" for local changes; None without git."""
    top = _git("rev-parse", "--show-toplevel")
    # Only this repo's own checkout counts. A copied component folder inside the
    # user's config repo would otherwise report that repo's commit.
    if top is None or Path(top).resolve() != _REPO:
        return None
    sha = _git("rev-parse", "--short=7", "HEAD")
    if not sha:
        return None
    # ":/" anchors the path at the repo root; git runs in components/levoit.
    changes = _git("status", "--porcelain", "--untracked-files=no", "--", ":/components")
    return f"{sha}.dirty" if changes else sha


def version() -> str:
    """Release plus commit, e.g. "1.7.0+3fa21c4"."""
    sha = commit()
    return f"{release()}+{sha}" if sha else release()
