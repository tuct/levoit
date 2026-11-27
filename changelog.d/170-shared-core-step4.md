Towards one protocol core for ESPHome and Matter
([#147](https://github.com/tuct/levoit/issues/147)), step 4: `components/levoit`
is an ESP-IDF component as well (`CMakeLists.txt`, `idf_component.yml`), so a
firmware that is not ESPHome pulls the protocol core in with the component
manager; `tests/idf` builds that way in CI. Nothing changes for an ESPHome
build
