# Sprout audio files for LittleFS

Files in this folder are built into the `assets` LittleFS partition when you run `esphome run`.

They must be extracted from your own firmware dump before flashing.


## Expected files

| File | Sound |
|------|-------|
| `100001.ogg` | Summer rain |
| `100002.mp3` | Gentle sea wave |
| `100003.mp3` | Sound 03 |
| `100004.mp3` | Sound 04 |
| `100005.mp3` | Sound 05 |
| `100006.mp3` | Insects chirp by the stream |
| `100007.mp3` | Sound 07 |
| `100008.mp3` | Sound 08 |
| `100009.mp3` | Morning seaside |
| `1000010.mp3` | Sound 10 |
| `1000011.mp3` | Sound 11 |
| `1000012.mp3` | Sound 12 |
| `1000013.mp3` | Sound 13 |
| `1000014.mp3` | Sound 14 |
| `1000015.mp3` | Sound 15 |
| `bgm.mp3` | UI background music |
| `switch.mp3` | Button click sound effect |

Only files present here will be included in the LittleFS image — missing files are silently skipped.

## File size

Keep files small — the `assets` partition is **2 MB** total and files are loaded fully into RAM before decoding.

- Target: **< 500 KB per file**
- Re-encode if needed: `ffmpeg -i input.mp3 -b:a 32k -ar 32000 -ac 1 output.mp3`
- The pre-loaded files from the original firmware are already small (mono, low bitrate) around 500KB
