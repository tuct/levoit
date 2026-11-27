Levoit UART frame handling
a command the MCU did not carry out (Display in Sleep on the Core 300S `01 32 40`
status) no longer stays on the entity: the first unchanged status push more than
1.5 s after a command is decoded. Pushes inside that window stay skipped, so fast
pushers do not flip the entity back. All models
