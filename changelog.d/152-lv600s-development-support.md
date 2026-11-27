LV600S humidifier (development)
local ESPHome support for the LUH-A602S-WEUR as a separate `lv600s_humidifier`
component: fixed-layout UART at 9600 baud, shared frame encoding, and an
original-ESP32 example. Settings writes require confirmed appliance power.
See `devices/levoit-lv600s/uart/` for the status, settings and timer frames.
The goal is `model: LV600S` once the shared protocol core exists
