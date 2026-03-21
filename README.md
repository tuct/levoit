# Tuct - esphome stuff

This is a collection of esphome projects and components that i build over the years.

Mostly DIY esphome devices, but also some some components and esphome based custom firmware

## ESPHome Components

* [Levoit](./components/levoit) – Generic Levoit ESPHome component supporting Core (200/300/400) and Vital (100/200) series with UART communication, filter tracking, and multi-entity support
* ~~[Sensirion Sen66](./components/sen6x)~~ – **DEPRECATED** Integration for the Sensirion Sen66 air quality sensor
* ~~[Levoit Core300s](./components/core300s)~~ – **DEPRECATED** (use generic Levoit component instead)
* ~~[Levoit Core400s](./components/core400s)~~ – **DEPRECATED** (use generic Levoit component instead)

## Project - Free Levoit Air Purifiers

Collection of custom ESPHome firmware and hardware projects for Levoit air purifiers, eliminating cloud dependency and enabling native Home Assistant integration.

**Supported Models:**

* [Core 300S](./projects/free-levoit/levoit-core300s) – Custom firmware (no hardware modifications needed)
* [Core 400S](./projects/free-levoit/levoit-core400s) – Custom firmware (no hardware modifications needed)
* [Vital 100S](./projects/free-levoit/levoit-vital100s) – Custom firmware (no hardware modifications needed)
* [LV-PUR 131S](./projects/free-levoit/levoit-lv131s) – Custom firmware + ESP32-C3 MCU upgrade + PM5003 sensor upgrade (hardware hack; plays Doom!)
* [Mini](./projects/free-levoit/levoit-mini) – Custom PCB, 3D-printed parts, and firmware (fully reversible modification)

Soon:
* [Core 200s](./projects/free-levoit/levoit-core200s) – Custom Firmware - WIP
* [Core 600s](./projects/free-levoit/levoit-core600s) – Custom Firmware - WIP
* [Vital 200S](./projects/free-levoit/levoit-vital200s) – Custom firmware (no hardware modifications needed)

**Key Features Across All Models:**
- Full Home Assistant integration via native ESPHome API (no cloud required)
- Fan control with Manual, Auto, and Sleep modes
- CADR-based filter lifetime tracking with configurable replacement intervals
- Filter low threshold binary sensor and one-touch reset button
- Real-time air quality and PM2.5 monitoring
- Display control and WiFi LED management

**Getting Started:**

See [Free Levoit Project Documentation](./projects/free-levoit/README.md) for complete details including:
- **Installation Methods** – 3 hardware approaches (reuse original ESP, dual ESP with switch, replace module)
- **UART Protocol Reference** – Frame format, checksums, TLV encoding for Vital series, command/response structures
- **Model-Specific Guides** – Disassembly, flash procedures, Quick Facts (MCU versions, speeds, CADR ratings) for each model
- **Component Integration** – Entity types, filter tracking calculations, configuration examples

**Community Resources:**
* [Vital 200S/Pro ESPHome](https://github.com/targor/levoit_vital/) – Extended Vital series support
* [Core 300S Alternative](https://github.com/mulcmu/esphome-levoit-core300s) – Community project
* [ESPHome Levoit Integration](https://github.com/acvigue/esphome-levoit-air-purifier) – Reference implementation


## Other Projects

* [aqMood](./projects/aqMood) – Emotionally responsive air quality mood light with Sen66 sensor integration (custom PCB, 3D parts)
* [Smart Control for Prusa Enclosure](./projects/smart-control-for-prusa-enclosure) – Enclosure climate monitoring and control (ESPHome, 3D parts)
* [Tiny Wash & Cure](./projects/tiny-wash-and-cure) – 3D printer support device control (custom PCB)
* [Automated Iris](./projects/automated-iris) – Automated aperture/iris control (ESPHome, 3D parts)


## TODO

* Add README to tiny-wash-and-cure project
* Add README to automated-iris project
* Expand aqMood documentation for wall-mounted variants


