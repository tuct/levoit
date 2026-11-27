[levoit][enhancement] Filter life
`LevoitDevice` accrues wear as seconds at full CADR (`get_filter_seconds`),
saved under `levoit_filter_seconds` once per completed hour and in `shutdown()`,
seeded once from `levoit_used_cadr`; `get_used_cadr` and the reset keep their
meaning
