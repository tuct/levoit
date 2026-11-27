#pragma once
// Logging for the protocol sources (*_status.cpp, *_commands.cpp, decoder.cpp,
// tlv.cpp), so they build with and without ESPHome: ESPHome's log macros in an
// ESPHome build, ESP-IDF's in a plain IDF project (a Matter firmware), and
// stderr on a host (tests/host). All three take ESP_LOGx(tag, format, ...).
// See docs/shared-core.md.
#if __has_include("esphome/core/log.h")
#include "esphome/core/log.h"
#elif __has_include("esp_log.h")
#include "esp_log.h"
#elif defined(LEVOIT_HOST_QUIET)
// Host build without log output (tests). The arguments are still evaluated,
// so a variable used only in a log line stays used.
#include <cstdio>
inline void levoit_log_sink_(const char *, const char *, ...) {}
#define LEVOIT_HOST_LOG(level, tag, fmt, ...) levoit_log_sink_(tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, ...) LEVOIT_HOST_LOG("E", tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) LEVOIT_HOST_LOG("W", tag, __VA_ARGS__)
#define ESP_LOGI(tag, ...) LEVOIT_HOST_LOG("I", tag, __VA_ARGS__)
#define ESP_LOGD(tag, ...) LEVOIT_HOST_LOG("D", tag, __VA_ARGS__)
#define ESP_LOGV(tag, ...) LEVOIT_HOST_LOG("V", tag, __VA_ARGS__)
#else
#include <cstdio>
#define LEVOIT_HOST_LOG(level, tag, fmt, ...) \
  std::fprintf(stderr, "[%s][%s] " fmt "\n", level, tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, ...) LEVOIT_HOST_LOG("E", tag, __VA_ARGS__)
#define ESP_LOGW(tag, ...) LEVOIT_HOST_LOG("W", tag, __VA_ARGS__)
#define ESP_LOGI(tag, ...) LEVOIT_HOST_LOG("I", tag, __VA_ARGS__)
#define ESP_LOGD(tag, ...) LEVOIT_HOST_LOG("D", tag, __VA_ARGS__)
#define ESP_LOGV(tag, ...) LEVOIT_HOST_LOG("V", tag, __VA_ARGS__)
#endif
