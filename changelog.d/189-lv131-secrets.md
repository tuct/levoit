[project][fix] LV-PUR131 config
the fallback hotspot reads the `wifi_ap_password` secret like every other device (it read `ap_password`), and the folder gets a `secrets-example.yaml`; rename the secret in an existing `secrets.yaml`
