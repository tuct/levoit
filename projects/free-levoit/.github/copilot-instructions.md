# Copilot Instructions for Free Levoit ESPHome Projects

## Project Overview
This repository contains custom ESPHome firmware and hardware projects for various Levoit air purifiers, enabling local control and Home Assistant integration. Each model (Core 300S, Core 400S, Vital 100S, LV-PUR 131S, Mini) has its own folder with YAML configs, hardware notes, and model-specific instructions.

## Architecture & Key Patterns
- **YAML-Driven:** All device logic is in ESPHome YAML files (e.g., `levoit-vital100s/vital100-custom.yaml`). These define sensors, controls, automations, and custom effects.
- **Component Reuse:** Common logic is abstracted in the [components/levoit](../../components/levoit/) directory (not present in this workspace snapshot, but referenced in docs).
- **Model Folders:** Each model (e.g., `levoit-vital100s/`, `levoit-core300s/`) contains:
  - Main YAML config(s)
  - `README.md` with hardware and usage notes
  - Example and real `secrets.yaml` for WiFi/API keys
  - Images, 3D parts, or PCB files as needed

## Developer Workflows
- **Build/Flash:**
  - Use `esphome run <model>/<config>.yaml` to build and flash firmware.
  - For first-time flash, connect via UART (see model README for pinout).
- **Secrets:**
  - Place WiFi/API credentials in `secrets.yaml` (copy from `secrets-example.yaml`).
- **Testing:**
  - YAML is validated and compiled by ESPHome; run `esphome run` to check for errors.
  - No traditional unit tests; rely on ESPHome logs and Home Assistant integration for validation.
- **Debugging:**
  - Use ESPHome logs (`esphome logs <config>.yaml`) for runtime debugging.
  - Many automations use `lambda:` blocks (C++), so C++/ESPHome warnings may appear—fix deprecated API usage (e.g., use `.current_option()` for selects).

## Project-Specific Conventions
- **Selects & Modes:**
  - Use ESPHome `select` components for user modes (e.g., Light Mode, Timer Mode).
  - Always use `.current_option()` (not `.state`) for select state in lambdas (future-proof for ESPHome 2026+).
- **Button Handling:**
  - Use `on_click` and `on_multi_click` for physical button logic (see `btn_display` for cycling modes).
- **Custom Effects:**
  - Addressable LED effects (e.g., AQI color) are implemented with `addressable_lambda` in YAML.
- **Globals:**
  - Use `globals:` for persistent state (e.g., filter usage, child lock).
- **Hardware Integration:**
  - Each model's README documents required hardware mods, pinouts, and disassembly steps.

## Integration Points
- **Home Assistant:**
  - All entities are exposed via ESPHome API for Home Assistant discovery.
- **WiFi/AP:**
  - Devices fall back to AP mode if WiFi fails (see `wifi:` block in YAML).

## Example: Cycling Light Mode
```yaml
on_multi_click:
  - timing:
      - ON for at least 2.1s
    then:
      - lambda: |-
          auto sel = id(light_mode);
          auto opts = sel->traits.get_options();
          int idx = 0, found = 0;
          for (size_t i = 0; i < opts.size(); i++) {
            if (std::string(opts[i]) == sel->current_option()) { idx = i; found = 1; break; }
          }
          if (!found) idx = 0;
          idx = (idx + 1) % opts.size();
          sel->make_call().set_option(opts[idx]).perform();
```

## Key Files & Directories
- `levoit-*/vital100-custom.yaml`, `levoit-*/README.md`: Main config and docs per model
- `secrets.yaml`, `secrets-example.yaml`: Credentials (never commit real secrets)
- `photos/`, `images/`, `3dparts/`, `PCB/`: Hardware and reference assets

## Do/Don't for AI Agents
- **Do:**
  - Use `.current_option()` for selects in lambdas
  - Follow model-specific README for hardware and config details
  - Reference and extend YAML patterns as shown
- **Don't:**
  - Use deprecated ESPHome APIs (e.g., `.state` for selects)
  - Assume all models have the same hardware—check each folder's README
  - Commit real secrets
