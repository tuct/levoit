[← Back to ESPHome Projects](../README.md)

# ESPHome Components

Collection of custom ESPHome components developed for various projects.

## SEN66
Just because i didn't wanted to wait for the official component after getting my SEN66 evaluation kit!

Only works with SEN66.

### Differences SEN5X to SEN6X
* Each type SEN65,SEN66,... has a dedicated READ command
* getProductName is not working (on my device at least)
* Temp/Hum accelaration has changed from 3 modes to fine params (not implemented)
* Auto clean has been removed, guess should be triggered from outside now