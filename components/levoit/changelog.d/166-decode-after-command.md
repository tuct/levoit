UART frame handling
after an `on_*_command`, unchanged status pushes stay skipped for 1.5 s while the
MCU applies it; the first unchanged push after that is decoded once and replaces
the requested value in the device state if the MCU did not carry the command out.
Host test with a Core 300S `01 32 40` frame
