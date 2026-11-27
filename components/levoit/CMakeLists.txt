# The Levoit protocol core as an ESP-IDF component, for firmware other than
# ESPHome (a Matter firmware, docs/shared-core.md step 4). ESPHome ignores this
# file: it copies the sources into its own build. Only the core is listed; the
# ESPHome glue (levoit.cpp and the platform directories) needs ESPHome.
idf_component_register(
    SRCS
        "levoit_device.cpp"
        "decoder.cpp"
        "core_status.cpp"
        "vital_status.cpp"
        "sprout_status.cpp"
        "superior_status.cpp"
        "core_commands.cpp"
        "vital_commands.cpp"
        "sprout_commands.cpp"
        "superior_commands.cpp"
        "tlv.cpp"
        "levoit_message.cpp"
    INCLUDE_DIRS "."
    REQUIRES log
)
