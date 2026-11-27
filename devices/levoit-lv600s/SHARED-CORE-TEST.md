# LV600S shared-core hardware verification

Use an EU LUH-A602S-WEUR with MCU 1.0.56 and the original ESP32-SOLO-1C.
Record the source commit, ESPHome version, MCU version and test result for
each row. The capture set under [uart/](uart/README.md) provides the baseline.

For an existing secure installation, build with its previous partition table,
device identity, encrypted API/OTA credentials and setup recovery profile.
Before uploading, compare the partition table with the running firmware,
verify access through its encrypted API, and check the SHA-256 of the trial
image and known-good rollback image. Use the authenticated OTA path. Run
the tests with the appliance reassembled. Keep the tank filled except
during the water and tank checks.

| Step | Action and expected result |
|---|---|
| Boot and standby | Confirm MCU version, humidity, temperature, tank and fault status. The device stays off and the display stays dark. With no timer, duration and remaining both show **Off**. |
| Power, mode and mist | Switch on from the HA panel and verify the physical display and reported power. Try Manual, Auto (Humidity) and Sleep. Select a mist level while in Auto and again from standby. Check that the fan enters Manual and reports the confirmed level. Test one level through the ESPHome fan and one through the HA panel. |
| Warm mist and display | Select warmth 0–3 and toggle the display while running in Manual. Check confirmed entities and the physical indicators. Check that Sleep blanks the display. |
| Target humidity | Save a target of 66% in Manual, switch to Auto, and verify that the MCU reports 66%. Repeat from the appliance buttons. Restore the original setpoint afterward. |
| Startup presets | In standby save warmth 2 and a 1-minute timer. Select mist level 5 on the fan or the panel and confirm that the device powers on in Manual and applies the settings. Check the timer runs once and ends in standby. |
| Timer cancellation | Start a timer while running, verify duration/countdown, and set the minutes control to 0. Both timer entities should show Off after the MCU report. Also clear a queued timer while still in standby. |
| Water, tank and stale status | In standby remove and reinsert the tank, then test with an empty tank. Check the HA indicators, and verify that unsafe writes are blocked. Refill and confirm normal controls return. Failures caused by lost UART status are also covered by the host test. |
| Return to baseline | Restore the previous firmware and settings after the trial. Confirm encrypted API access and basic controls again. Review the HA registry for matching existing entity names and IDs. |

Do the physical factory reset as a separate test with the setup credentials
at hand. Keep any raw captures and build artifacts private.
