Levoit UART frame handling
after a command the next status push is decoded even when it repeats the last
payload, so a command the MCU did not carry out (Display in Sleep on the Core 300S
`01 32 40` status) no longer stays on the entity. All models
