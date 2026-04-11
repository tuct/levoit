# Sprout audio files for LittleFS

Files in this folder are built into the `assets` LittleFS partition when you run `esphome run`.

## Pre-loaded (from firmware dump)

| File | Sound |
|------|-------|
| `100001.ogg` | Summer rain |
| `100002.mp3` | Gentle sea wave |
| `100006.mp3` | Insects chirp by the stream |
| `100009.mp3` | Morning seaside |
| `1000012.mp3` | Sound 12 |
| `bgm.mp3` | UI background music |
| `switch.mp3` | Button click sound effect |

## Adding more sounds

Download remaining sounds via the original Levoit app (see Sprout README for procedure),
then copy them here from `../assets_extracted/`.

The other sound filenames are: `100003.mp3` through `1000015.mp3` (except the ones above).

## How it works

`build_assets.py` runs as a PlatformIO pre-build script. It:
1. Reads all files from this folder
2. Builds a LittleFS image using `littlefs-python`
3. Flashes it to the `assets` partition (offset `0x320000`) alongside the firmware

Install the dependency once: `pip install littlefs-python`
