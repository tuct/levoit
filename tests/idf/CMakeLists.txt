# A minimal ESP-IDF project that pulls the Levoit protocol core in as a
# component through the IDF component manager (main/idf_component.yml) and
# builds it with ESP-IDF's own warning settings. CI builds it
# (.github/workflows/idf-component.yml); it is never flashed. What it proves:
# components/levoit is a usable component for a firmware that is not ESPHome,
# which is what a Matter firmware needs (docs/shared-core.md, step 4).
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(levoit-core-check)
