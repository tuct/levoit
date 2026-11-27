Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)): `LevoitDevice` exposes per-model
capabilities (`speed_count`, `has_auto`, `has_air_quality`, `display_name`) and
`set_filter_percent`, so a firmware that is not ESPHome reads them instead of
keeping its own table. No change for ESPHome.
