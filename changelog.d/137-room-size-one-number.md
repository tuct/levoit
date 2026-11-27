* **Room size: one number to set** (Core 300S / 400S / 600S, Vital 100S /
  200S). Changing *Auto Mode Room Size Preset* while Room Size is the active
  auto mode now sends it to the purifier at once; before, it only took effect
  the next time Room Size was picked. *Auto Mode Room Size*, which shows what
  the purifier reports (0 outside Room Size), moves to *Diagnostic* in Home
  Assistant. Same entity, same name: automations keep working. The Sprout
  keeps it as a setting, since it has no Preset
  ([#137](https://github.com/tuct/levoit/pull/137))
