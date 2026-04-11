# levoit_audio — LittleFS I2S audio for Sprout

Plays white noise MP3/OGG files from the ESP32 `assets` LittleFS partition via the onboard I2S amplifier.

## Setup

Download the two single-header decoders and place them next to `levoit_audio.h`:

```bash
# MP3 decoder
curl -L https://raw.githubusercontent.com/mackron/dr_libs/master/dr_mp3.h -o dr_mp3.h

# OGG/Vorbis decoder
curl -L https://raw.githubusercontent.com/nothings/stb/master/stb_vorbis.c -o stb_vorbis.c
```

## Usage from ESPHome lambda

```yaml
esphome:
  includes:
    - levoit_audio/levoit_audio.h

on_boot:
  then:
    - lambda: levoit_audio_setup();

# Play sound index 0 when white noise switch turns on
switch:
  - platform: template
    name: "White Noise"
    turn_on_action:
      - lambda: levoit_audio_play(0);   # non-blocking — returns immediately
    turn_off_action:
      - lambda: levoit_audio_stop();
```

## Notes

- `levoit_audio_play()` is **non-blocking** — decoding runs on a background FreeRTOS task (stack 8 KB, priority 5). The ESPHome main loop is not blocked.
- Calling `levoit_audio_play()` while a sound is already playing stops the current track first, then starts the new one.
- `levoit_audio_stop()` sets a flag — playback stops at the next decode chunk boundary (< one DMA buffer, ~16 ms).
- Files are loaded fully into heap before decoding — largest file should be < 400 KB, well within ESP32 SRAM headroom.
- `SOUND_FILES[]` is already populated with the confirmed filenames from the firmware dump. Indices without a pre-loaded file require downloading via the original app first — see the [Sprout README](../README.md) for the download procedure.
- Mono output: stereo sources are mixed down to mono before writing to I2S (amp is mono).
