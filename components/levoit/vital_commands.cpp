#include "vital_commands.h"
#include "levoit_message.h"
#include "levoit.h"
#include "number/levoit_number.h"
#include "select/levoit_select.h"
#include "switch/levoit_switch.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG_VITAL_CMD = "levoit.vital_cmd";

    std::vector<uint8_t> build_vital_command(Levoit *self, CommandType cmd)
    {
      std::vector<uint8_t> msg_type;
      std::vector<uint8_t> payload;

      switch (cmd)
      {
      case CommandType::setDeviceON:
        msg_type = {0x02, 0x00, 0x50};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDeviceOFF:
        msg_type = {0x02, 0x00, 0x50};
        payload = {0x01, 0x01, 0x00};
        break;

      case CommandType::setDeviceFanLvl1:
        msg_type = {0x02, 0x03, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDeviceFanLvl2:
        msg_type = {0x02, 0x03, 0x55};
        payload = {0x01, 0x01, 0x02};
        break;

      case CommandType::setDeviceFanLvl3:
        msg_type = {0x02, 0x03, 0x55};
        payload = {0x01, 0x01, 0x03};
        break;

      case CommandType::setDeviceFanLvl4:
        msg_type = {0x02, 0x03, 0x55};
        payload = {0x01, 0x01, 0x04};
        break;

      case CommandType::setFanModeAuto:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x01, 0x01, 0x02};
        break;

      case CommandType::setFanModeSleep:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setFanModeManual:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x01, 0x01, 0x00};
        break;

      case CommandType::setFanModePet:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x01, 0x01, 0x05};
        break;

      case CommandType::setLightDetectOn:
        msg_type = {0x02, 0x11, 0x55};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setLightDetectOff:
        msg_type = {0x02, 0x11, 0x55};
        payload = {0x01, 0x01, 0x00};
        break;

      case CommandType::setDisplayOn:
        msg_type = {0x02, 0x04, 0x55};
        payload = {0x01, 0x01, 0x64};
        break;

      case CommandType::setDisplayOff:
        msg_type = {0x02, 0x04, 0x55};
        payload = {0x01, 0x01, 0x00};
        break;

      case CommandType::setDisplayLockOn:
        msg_type = {0x02, 0x40, 0x51};
        payload = {0x01, 0x01, 0x01};
        break;

      case CommandType::setDisplayLockOff:
        msg_type = {0x02, 0x40, 0x51};
        payload = {0x01, 0x01, 0x00};
        break;

      case CommandType::setAutoModeDefault:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x02, 0x01, 0x00, 0x03, 0x02, 0x00, 0x00};
        break;

      case CommandType::setAutoModeQuiet:
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x02, 0x01, 0x01, 0x03, 0x02, 0x00, 0x00};
        break;

      case CommandType::setAutoModeEfficient:
      {
        msg_type = {0x02, 0x02, 0x55};
        auto *num = self->get_number(NumberType::EFFICIENCY_ROOM_SIZE);
        if (num != nullptr)
        {
          // Convert m² → raw MCU value: 1 m² = 10.764 sq ft, Vital MCU uses sq_ft × 1.3
          float m2 = num->state;
          uint32_t room_size = static_cast<uint32_t>(m2 * 10.764f * 1.3f + 0.5f);
          uint8_t size_low = room_size & 0xFF;
          uint8_t size_high = (room_size >> 8) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setAutoModeEfficient: %.0f m² -> raw=%u (0x%02X 0x%02X)",
                   m2, room_size, size_low, size_high);

          payload = {0x02, 0x01, 0x02, 0x03, 0x02, size_low, size_high};
        }
        else
        {
          payload = {0x02, 0x01, 0x02, 0x03, 0x02, 0xAD, 0x01};
        }
        break;
      }

      case CommandType::setTimerStop:
        msg_type = {0x02, 0x19, 0x50};
        payload = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00};
        self->stop_timer();
        break;

      case CommandType::setTimerMinutes:
      {
        msg_type = {0x02, 0x19, 0x50};
        auto *num = self->get_number(NumberType::TIMER);
        if (num != nullptr)
        {
          uint32_t secs = static_cast<uint32_t>(num->state) * 60;
          uint8_t b0 = (secs >> 0) & 0xFF;
          uint8_t b1 = (secs >> 8) & 0xFF;
          uint8_t b2 = (secs >> 16) & 0xFF;
          uint8_t b3 = (secs >> 24) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",
                   (unsigned)secs, b0, b1, b2, b3);

          payload = {0x01, 0x04, b0, b1, b2, b3};

          if (secs > 0)
            self->start_timer();
          else
            self->stop_timer();
        }
        break;
      }

      case CommandType::requestTimerStatus:
        msg_type = {0x02, 0x1A, 0x50};
        payload = {};
        break;

      case CommandType::setBulkPrefs:
      {
        // Bulk write for sleep / quick-clean / white-noise / daytime
        // preference clusters under CMD 02 02 55, tags 0x04..0x0F.
        // Discovered via stock-firmware disassembly (see
        // docs/STOCK_FIRMWARE_FINDINGS.md "Sleep / quick-clean / white-
        // noise / daytime bulk write") and verified on MCU 2.0.0.
        // Protocol summary:
        //  - SET tag = status TLV - 0x14 (monotonic mapping).
        //  - Tag 0x04 (sleep_type) is the gate: when 0x00 (Default), the
        //    MCU silently drops tags 0x05..0x0F. To change any of those
        //    fields the user must first switch sleep_type to a non-
        //    Default value (Custom1/Custom2).
        //  - Storage is a single shared store (not per-preset), so non-
        //    Default writes overwrite the stored values regardless of
        //    which "preset" they're nominally associated with.
        msg_type = {0x02, 0x02, 0x55};

        const auto &c = self->get_bulk_prefs();
        if (!c.valid()) {
          ESP_LOGW(TAG_VITAL_CMD,
                   "setBulkPrefs: cache not fully populated (seen_mask=0x%03X) "
                   "— refusing to send.", (unsigned)c.seen_mask);
          return {};  // empty -> sendCommand skips transmission
        }

        // Start every field at its cached value; override only where an
        // entity exists AND has been published. LevoitSelect/LevoitNumber
        // call publish_state() BEFORE on_*_command, so by the time this
        // builder runs, the entity reflects the new value.
        uint8_t  sleep_type = c.sleep_type;
        uint8_t  qc_enabled = c.qc_enabled;
        uint16_t qc_min     = c.qc_min;
        uint8_t  qc_fan     = c.qc_fan;
        uint8_t  wn_enabled = c.wn_enabled;
        uint16_t wn_min     = c.wn_min;
        uint8_t  wn_fan     = c.wn_fan;
        uint8_t  sleep_fan  = c.sleep_fan;
        uint16_t sleep_min  = c.sleep_min;
        uint8_t  dt_enabled = c.dt_enabled;
        uint8_t  dt_mode    = c.dt_mode;
        uint8_t  dt_level   = c.dt_level;

        if (auto *sel = self->get_select(SelectType::SLEEP_PREFERENCE)) {
          if (auto idx = sel->active_index()) sleep_type = (uint8_t)idx.value();
        }
        if (auto *n = self->get_number(NumberType::SLEEP_FAN_LEVEL)) {
          if (n->has_state()) sleep_fan = (uint8_t)n->state;
        }
        if (auto *n = self->get_number(NumberType::SLEEP_MODE_MIN)) {
          if (n->has_state()) sleep_min = (uint16_t)n->state;
        }
        // QC cluster (Stage 4). Optimistic publish in LevoitSwitch / LevoitNumber
        // means the entity reflects the new value by the time we read it here.
        if (auto *sw = self->get_switch(SwitchType::QUICK_CLEAN)) {
          if (sw->has_state()) qc_enabled = sw->state ? 1 : 0;
        }
        if (auto *n = self->get_number(NumberType::QUICK_CLEAN_MIN)) {
          if (n->has_state()) qc_min = (uint16_t)n->state;
        }
        if (auto *n = self->get_number(NumberType::QUICK_CLEAN_FAN_LEVEL)) {
          if (n->has_state()) qc_fan = (uint8_t)n->state;
        }
        // DT cluster (Stage 5).
        if (auto *sw = self->get_switch(SwitchType::DAYTIME_ENABLED)) {
          if (sw->has_state()) dt_enabled = sw->state ? 1 : 0;
        }
        if (auto *sel = self->get_select(SelectType::DAYTIME_FAN_MODE)) {
          if (auto idx = sel->active_index()) dt_mode = (uint8_t)idx.value();
        }
        if (auto *n = self->get_number(NumberType::DAYTIME_FAN_LEVEL)) {
          if (n->has_state()) dt_level = (uint8_t)n->state;
        }
        // WN cluster is cache-only by design — Vital 200S Pro has no WN
        // hardware, but the MCU still requires those 3 fields in every
        // bulk write or the parser silently drops the whole frame.

        ESP_LOGD(TAG_VITAL_CMD,
                 "setBulkPrefs: sleep[type=%u fan=%u min=%u] "
                 "QC[en=%u fan=%u min=%u] WN[en=%u fan=%u min=%u] "
                 "DT[en=%u mode=%u lvl=%u]",
                 sleep_type, sleep_fan, sleep_min,
                 qc_enabled, qc_fan, qc_min,
                 wn_enabled, wn_fan, wn_min,
                 dt_enabled, dt_mode, dt_level);
        if (sleep_type == 0) {
          ESP_LOGW(TAG_VITAL_CMD,
                   "setBulkPrefs: sleep_type=0 (Default) — MCU will drop "
                   "writes to tags 0x05..0x0F. Only the type byte applies.");
        }

        const uint8_t qc_min_lo    = qc_min    & 0xFF, qc_min_hi    = (qc_min    >> 8) & 0xFF;
        const uint8_t wn_min_lo    = wn_min    & 0xFF, wn_min_hi    = (wn_min    >> 8) & 0xFF;
        const uint8_t sleep_min_lo = sleep_min & 0xFF, sleep_min_hi = (sleep_min >> 8) & 0xFF;

        payload = {
            0x04, 0x01, sleep_type,
            0x05, 0x01, qc_enabled,
            0x06, 0x02, qc_min_lo, qc_min_hi,
            0x07, 0x01, qc_fan,
            0x08, 0x01, wn_enabled,
            0x09, 0x02, wn_min_lo, wn_min_hi,
            0x0A, 0x01, wn_fan,
            0x0B, 0x01, sleep_fan,
            0x0C, 0x02, sleep_min_lo, sleep_min_hi,
            0x0D, 0x01, dt_enabled,
            0x0E, 0x01, dt_mode,
            0x0F, 0x01, dt_level,
        };
        break;
      }

      case CommandType::setWifiLedOn:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x01, 0x02, 0x02, 0x7D, 0x00, 0x03, 0x02, 0x7D, 0x00, 0x04, 0x01, 0x00};
        break;

      case CommandType::setWifiLedOff:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x00, 0x02, 0x02, 0xF4, 0x01, 0x03, 0x02, 0xF4, 0x01, 0x04, 0x01, 0x00};
        break;

      case CommandType::setWifiLedBlinking:
        msg_type = {0x02, 0x18, 0x50};
        payload = {0x01, 0x01, 0x02, 0x02, 0x02, 0xF4, 0x01, 0x03, 0x02, 0xF4, 0x01, 0x04, 0x01, 0x00};
        break;

      default:
        ESP_LOGW(TAG_VITAL_CMD, "Command not implemented: %s", command_type_to_string(cmd));
        return {};
      }

      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

  } // namespace levoit
} // namespace esphome
