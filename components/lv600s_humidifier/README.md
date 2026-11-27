# LV600S humidifier (development)

Separate fixed-layout component for LV600S, 9600 baud. The `levoit` purifier/TLV
component and the Superior 6000S model are unchanged.

See the [device guide](../../devices/levoit-lv600s) and
[protocol notes](../../devices/levoit-lv600s/PROTOCOL.md). The parent block supports
optional sensors, binary sensors and text sensors; controls use ordinary ESPHome
template entities from the example. It does not create a fake fan or filter.
Its `model: LV600S` and `esp_version` use the shared repository version source.

Validation covers a full ESP32 single-core build and C++ protocol, queue and
state tests. One EU LV600S with MCU 1.0.56 ran this component; the
[device README](../../devices/levoit-lv600s/README.md#verification) lists what
was checked on it. CI builds at ESPHome 2026.6.0 (minimum) and latest.

This component is a stepping stone. The LV600S speaks the same Levoit frame as
the purifiers (`A5`, control, sequence, length, inverted-sum checksum,
`01 cmd_lo cmd_hi 00` body); only its payload is fixed layout instead of TLV,
at 9600 baud. The goal is `model: LV600S` inside the `levoit` component once
the shared protocol core from [#147](https://github.com/tuct/levoit/issues/147)
exists. Do not build on `lv600s_humidifier` as a stable name.

```sh
python components/lv600s_humidifier/tests/test_protocol.py
esphome compile components/lv600s_humidifier/tests/test-all-entities.yaml
```

The default native test enables ASan+UBSan. Set `LEVOIT_TEST_SANITIZERS=undefined`
only on hosts where the ASan runtime itself is broken; do not describe that as an
ASan success. The test never contacts a device.

## Unreleased

- Add the fixed-layout MCU protocol, confirmed setters, timer tracking and safety diagnostics for LV600S.
