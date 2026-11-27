* **Core 300S on the `01 32 40` status: Display reads off in Sleep and while
  off.** The panel is dark then, but the display setting byte stays on, so
  *Display* showed on. It now shows what the panel does. The setting is
  kept: leaving Sleep or turning on brings the display back as before.
  Frames in [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)
  ("sleep", "power off")
