[← Back to Components](../README.md)

# Sensirion Sen6X ESPHome Component

Integration for the Sensirion Sen6X air quality sensor.

## SEN66
Just because i didn't wanted to wait for the official component after getting my SEN66 evaluation kit!

Only tested with SEN66  (Eval Kit and released version)

### Differences SEN5X to SEN6X
* Each type SEN65,SEN66,... has a dedicated READ command
* Temp/Hum accelaration has changed from 3 modes to fine params 
* Auto clean has been removed, guess should be triggered from outside now

### Open
* New commands for Co2 and Pressure and Altidude
* auto clean mode
