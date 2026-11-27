* **Core 300S with the `01 32 40` status** (a LAP-C302S-WGC, MCU 1.0.0) now
  reports its state: the 19-byte status is decoded and asked for at boot, a
  filter reset on the panel (Sleep held 3 s) resets the filter life, and a
  new `power_button_held` binary sensor (Core models) follows a long press of
  the On/Off button. Capture in
  [`status_32_40.txt`](./devices/levoit-core300s/uart/status_32_40.txt)
  ([#140](https://github.com/tuct/levoit/pull/140),
  [#141](https://github.com/tuct/levoit/pull/141),
  [#142](https://github.com/tuct/levoit/pull/142),
  [#143](https://github.com/tuct/levoit/pull/143))
