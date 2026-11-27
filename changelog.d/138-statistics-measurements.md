* **Statistics for the remaining measurements**: the Superior 6000S humidity,
  temperature and MCU filter life, and the Philips filter sensors, get
  `state_class: measurement`, so Home Assistant keeps long-term statistics for
  them, as it already did for PM, AQI, CADR and filter life. The Superior ones
  also get their unit and device class (%, °C) ([#138](https://github.com/tuct/levoit/pull/138))
