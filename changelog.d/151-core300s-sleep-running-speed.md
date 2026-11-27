* **Core 300S on the `01 32 40` status: speed reads 1 in Sleep.** The fan showed
  the last manual speed in Sleep, often 3, though it runs slower than speed 1. It
  now reads the running speed, `00` in Sleep, as Auto already did, which the fan
  shows as its lowest speed. Frames in
  [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt) ("sleep")
