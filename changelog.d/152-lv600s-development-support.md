LV600S humidifier (development)
local ESPHome support for the LUH-A602S-WEUR as a separate `lv600s_humidifier`
component: fixed-layout UART control at 9600 baud, an original-ESP32 example,
and a wiring and protocol guide. Verified on one EU unit with MCU 1.0.56. The
goal is `model: LV600S` in `levoit` once the shared protocol core
([#147](https://github.com/tuct/levoit/issues/147)) exists
