UART frame handling
each `on_*_command` clears the status-push fingerprints, so the next push is
decoded and replaces the requested value in the device state when the MCU did not
carry the command out. Host test with a Core 300S `01 32 40` frame
