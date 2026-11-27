Core 300S on the `01 32 40` status
the device state keeps the manual speed last picked (`[5]`) as
`FanState::selected_speed` next to the running speed, for hosts other than the
ESPHome fan entity. Frames in
[`status_32_40.txt`](../../devices/levoit-core300s/uart/status_32_40.txt)
