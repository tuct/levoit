"""Prepare a builder file for a CI compile.

    python .github/scripts/builder_ci_config.py <builder.yaml> <out-dir>

Writes <out-dir>/<name>.yaml and <out-dir>/secrets.yaml:

* the component's GitHub source (type: git, ref: stable) becomes this checkout's
  components/ folder, so the build tests the code of the PR, not the last
  release
* dummy secrets for the keys the builder files read

Fails if the builder file does not have exactly one GitHub source of this repo,
so a format change in make-builder-yaml.ps1 cannot silently skip the swap.
"""

import base64
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

SOURCE = re.compile(
    r"^(?P<indent>[ \t]+)type: git\n"
    r"(?P=indent)url: https://github\.com/tuct/levoit\n"
    r"(?P=indent)ref: \S+\n",
    re.MULTILINE,
)

SECRETS = {
    "wifi_ssid": "ci-builder",
    "wifi_password": "ci-builder-password",
    "wifi_ap_password": "ci-builder-ap",
    # api: encryption: key must be 32 bytes, base64
    "api_key": base64.b64encode(bytes(range(32))).decode(),
    "ota_password": "ci-builder-ota",
}


def main() -> int:
    builder, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    text = builder.read_text(encoding="utf-8-sig").replace("\r\n", "\n")

    components = (REPO / "components").as_posix()

    def local(match: re.Match) -> str:
        indent = match.group("indent")
        return f"{indent}type: local\n{indent}path: {components}\n"

    text, count = SOURCE.subn(local, text)
    if count != 1:
        print(f"{builder}: expected one GitHub source of tuct/levoit, found {count}")
        return 1

    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / builder.name).write_text(text, encoding="utf-8")
    (out_dir / "secrets.yaml").write_text(
        "".join(f'{key}: "{value}"\n' for key, value in SECRETS.items()),
        encoding="utf-8",
    )
    print(f"{builder} -> {out_dir / builder.name} (source: {components})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
