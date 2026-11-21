[← Back](../../README.md)

# Eleva 300X — Saleae Logic 2 captures

ESP32-C3 ↔ MCU link of a stock Eleva 300X (Wi-Fi FW 1.0.08, MCU FW 1.0.13), 115200 8N1.
**Channel 0 = MCU TX, channel 1 = ESP TX**, 10 MS/s. Open them in Logic 2; the
[levoit_uart analyzer](../../../../logic2/levoit_uart) decodes the frames.
The protocol notes they back are in the [device README](../../README.md#uart-protocol-wip).

| Capture | Length | What was done |
|---------|--------|---------------|
| `speeds_modes.sal` | 60 s | Panel buttons: speed 2, 3, turbo, auto, sleep (no ESP commands, status only) |
| `app_commands.sal` | 90 s | App: speeds 1–4 (twice), auto, sleep, child lock on/off, light detection on/off |
| `app_commands2.sal` | 120 s | App: speeds 1–4, display off/on, child lock, AQI ring dim/bright, light detection, mute, auto → manual |
| `app_commands3.sal` | 120 s | App: auto → sleep, light detection on; display follows the light sensor |
| `app_commands4.sal` | 120 s | App: sleep → manual, light detection off, fast toggling of AQI ring and mute |
| `ring_mute_power.sal` | 30 s | App: mute and ring toggles (ends mute on, ring bright), power off → on → off |
| `auto_mode_settings.sal` | 30 s | App auto-mode settings; auto mode changes the fan level on its own |
| `quiet_period.sal` | 60 s | Quiet period settings (`5521` keys 05–08) |
| `adaptive_start.sal` | 30 s | Adaptive start toggles (`5521` key 02) |
| `boot_timesync.sal` | 60 s | Power cycle: MCU time request `5522`, boot sequence, local time sent; quiet window 22:00–06:00 → 21:30–06:30 |
| `sleep_settings.sal` | 90 s | Dynamic sleep on/off (`04`), quiet window change, adaptive start, dynamic manual (`03`) |
| `timer.sal` | 90 s | Timer 1 h, cancel, 30 min, cancel, 8 h, cancel (`5019` / `501a` / `501b`) |
| `quiet_light.sal` | 120 s | Modes, light detection on, quiet `06` 1→0, `05` 1→0; sensor covered (power key touched by accident at 82.7 s) |
| `quiet_light2.sal` | 120 s | Continuation of the quiet-by-light test |
| `quiet_light3.sal` | 90 s | Quiet on with trigger light, sensor covered → `5521 09=7`, fan 4→3, display off; uncovered → `09=8` |
| `filter_reset.sal` | 180 s | Filter reset on the panel (MCU → ESP `5505 03`), unlink + reset in the app (`03`, `04=0`) |
| `filter_reset2.sal` | 180 s | Power cycle with a non-genuine filter (boot `5505 04=0`), linking the filter in the app (`04=1`) |
| `filter_nongenuine_boot.sal` | 180 s | App "done replace" for main and carbon filter (nothing sent to the MCU), three boots |
| `filter_genuine_boot.sal` | 120 s | Idle after the genuine-filter boots |

The flash backup and the ESP debug-log captures are not committed: the backup
holds the device's factory keys, the logs its MAC address.
