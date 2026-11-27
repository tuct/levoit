LV600S humidifier (development)
local ESPHome support for the LUH-A602S-WEUR as a separate `lv600s_humidifier`
component: fixed-layout UART at 9600 baud, shared frame encoding, and an
original-ESP32 example. Settings writes require confirmed appliance power. The
goal is `model: LV600S` in `levoit` once the shared protocol core
([#147](https://github.com/tuct/levoit/issues/147)) exists
