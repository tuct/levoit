# Saleae Logic 2 HLA Python API Notes

## AnalyzerFrame + result_types

The format string in `result_types` requires the `data.` prefix to access frame fields:

```python
result_types = {
    "packet": {
        "format": "{{data.text}}"        # CORRECT
        # "format": "{{text}}"           # WRONG — renders empty
        # "format": "{{data}}"           # WRONG — renders [object Object]
        # "format": "{{value}}"          # WRONG — renders nothing
    }
}
```

Pass fields as a plain dict to `AnalyzerFrame`:

```python
return AnalyzerFrame("packet", start_time, end_time, {"text": "your string here"})
```

Multiple fields work too:

```python
result_types = {
    "packet": {
        "format": "{{data.address}} | {{data.count}} bytes | {{data.payload}}"
    }
}

return AnalyzerFrame("packet", start_time, end_time, {
    "address": "0x22",
    "count": 3,
    "payload": "01 02 03"
})
```

## Settings

Import and use `ChoicesSetting`, `StringSetting`, `NumberSetting` from `saleae.analyzers`:

```python
from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting

class MyHLA(HighLevelAnalyzer):
    my_setting = ChoicesSetting(label="My Setting", choices=("Option A", "Option B", "-"))
```

- Settings are class-level attributes
- Access at runtime via `self.my_setting` (returns the selected string)
- **Avoid Unicode** in choice strings (e.g. `→`) — use ASCII (`->`) to avoid silent load errors

## Restrict input analyzer

```python
class MyHLA(HighLevelAnalyzer):
    supported_analyzers = ["Async Serial"]
```

This filters the HLA to only appear when an Async Serial analyzer is selected as input.

## Receiving frames

```python
def decode(self, frame):
    if frame.type not in ("data", "byte"):
        return None
    byte = frame.data["data"][0]   # first byte of this frame
    ...
```

## Reserved / conflicting field names

Avoid these as data dict keys — they conflict with Logic 2 internals:

| Key      | Result            |
|----------|-------------------|
| `"data"` | `[object Object]` |
| `"text"` | empty (worked with `{{data.text}}`) |
| `"value"`| nothing rendered  |

Safe pattern: always use `{{data.fieldname}}` in format and `{"fieldname": ...}` in the dict.

## Extension display name

Set in `extension.json` — the `"name"` field is what shows in Logic 2's analyzer list:

```json
{
  "name": "Levoit UART Extractor",
  ...
}
```

The key inside `"extensions": {}` is an internal identifier only.
