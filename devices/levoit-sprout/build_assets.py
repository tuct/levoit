"""
Levoit Sprout — LittleFS asset builder
PlatformIO pre-build script.

Reads files from the `data/` folder next to the device yaml,
builds a LittleFS image, and registers it as an extra binary
to flash at the `assets` partition offset (0x320000).

Usage:
  - Place audio files in devices/levoit-sprout/data/
  - ESPHome build picks this up automatically via extra_scripts

Install dependency once:
  pip install littlefs-python
"""

Import("env")  # noqa: F821 — PlatformIO injects this

import os
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Config — must match partitions_custom.csv
# ---------------------------------------------------------------------------
ASSETS_PARTITION_OFFSET = 0x320000
ASSETS_PARTITION_SIZE   = 0x200000   # 2 MB
BLOCK_SIZE              = 4096

# data/ folder lives next to this script (i.e. next to the device yaml)
SCRIPT_DIR = Path(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR   = SCRIPT_DIR / "data"
IMAGE_OUT  = Path(env.subst("$BUILD_DIR")) / "assets_littlefs.bin"

# ---------------------------------------------------------------------------

def build_littlefs_image():
    try:
        from littlefs import LittleFS
    except ImportError:
        print("[assets] Installing littlefs-python ...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "littlefs-python"])
        from littlefs import LittleFS

    if not DATA_DIR.exists():
        print(f"[assets] data/ folder not found at {DATA_DIR} — skipping LittleFS build")
        return False

    files = [f for f in DATA_DIR.iterdir() if f.is_file()]
    if not files:
        print(f"[assets] data/ is empty — skipping LittleFS build")
        return False

    block_count = ASSETS_PARTITION_SIZE // BLOCK_SIZE
    fs = LittleFS(block_size=BLOCK_SIZE, block_count=block_count, read_size=128, prog_size=128)

    print(f"[assets] Building LittleFS image from {DATA_DIR}:")
    for f in sorted(files):
        with open(f, "rb") as src:
            with fs.open(f"/{f.name}", "wb") as dst:
                data = src.read()
                dst.write(data)
        print(f"  + {f.name:30s}  {len(data):>8,} bytes")

    IMAGE_OUT.parent.mkdir(parents=True, exist_ok=True)
    raw = bytes(fs.context.buffer)
    with open(IMAGE_OUT, "wb") as out:
        out.write(raw)

    print(f"[assets] Image: {IMAGE_OUT}  ({len(raw):,} bytes)")
    return True


def add_flash_target(source, target, env):
    """Merge the LittleFS image into the combined flash image after linking."""
    if not IMAGE_OUT.exists():
        return
    # esptool flash_args style — append offset + image path so esphome upload includes it
    env.Append(UPLOADERFLAGS=[
        hex(ASSETS_PARTITION_OFFSET),
        str(IMAGE_OUT),
    ])
    print(f"[assets] Added to upload: {hex(ASSETS_PARTITION_OFFSET)} {IMAGE_OUT.name}")


# Build image at the start of the compile phase
if build_littlefs_image():
    # Hook: inject into the upload command after the firmware is linked
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", add_flash_target)
