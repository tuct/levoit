#include "vital_commands.h"
#include "levoit_message.h"
#include "levoit.h"
#include "esphome/core/defines.h"
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

      case CommandType::setFanModeTurbo:
        // EverestAir top speed. Sent as a fan mode (0x04), not a fan level.
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x01, 0x01, 0x04};
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
        float m2;
        if (self->try_get_number_state(NumberType::EFFICIENCY_ROOM_SIZE, m2))
        {
          // Convert m² → raw MCU value: 1 m² = 10.764 sq ft, Vital MCU uses sq_ft × 1.3
          uint32_t room_size = static_cast<uint32_t>(m2 * 10.764f * 1.3f + 0.5f);
          uint8_t size_low = room_size & 0xFF;
          uint8_t size_high = (room_size >> 8) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setAutoModeEfficient: %.0f m² -> raw=%u (0x%02X 0x%02X)",
                   m2, (unsigned)room_size, size_low, size_high);

          payload = {0x02, 0x01, 0x02, 0x03, 0x02, size_low, size_high};
        }
        else
        {
          payload = {0x02, 0x01, 0x02, 0x03, 0x02, 0xAD, 0x01};
        }
        break;
      }

      case CommandType::setAutoModeEco:
        // EverestAir auto mode "Eco" = fan-mode byte 0x03 (vs 0x00 Default).
        msg_type = {0x02, 0x02, 0x55};
        payload = {0x02, 0x01, 0x03, 0x03, 0x02, 0x00, 0x00};
        break;

      case CommandType::setTimerStop:
        msg_type = {0x02, 0x19, 0x50};
        payload = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00};
        // Eleva 300X: the stock ESP always appends key 02 = 0 (meaning TBD)
        if (self->get_model() == ModelType::ELEVA300X)
          payload.insert(payload.end(), {0x02, 0x01, 0x00});
        self->stop_timer();
        break;

      case CommandType::setTimerMinutes:
      {
        msg_type = {0x02, 0x19, 0x50};
        float mins;
        if (self->try_get_number_state(NumberType::TIMER, mins))
        {
          uint32_t secs = static_cast<uint32_t>(mins) * 60;
          uint8_t b0 = (secs >> 0) & 0xFF;
          uint8_t b1 = (secs >> 8) & 0xFF;
          uint8_t b2 = (secs >> 16) & 0xFF;
          uint8_t b3 = (secs >> 24) & 0xFF;

          ESP_LOGD(TAG_VITAL_CMD, "setTimerMinutes: secs=%u bytes=%02X %02X %02X %02X",
                   (unsigned)secs, b0, b1, b2, b3);

          payload = {0x01, 0x04, b0, b1, b2, b3};
          // Eleva 300X: the stock ESP always appends key 02 = 0 (meaning TBD)
          if (self->get_model() == ModelType::ELEVA300X)
            payload.insert(payload.end(), {0x02, 0x01, 0x00});

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

        { size_t i; if (self->try_get_select_index(SelectType::SLEEP_PREFERENCE, i)) sleep_type = (uint8_t)i; }
        { float v; if (self->try_get_number_state(NumberType::SLEEP_FAN_LEVEL, v)) sleep_fan = (uint8_t)v; }
        { float v; if (self->try_get_number_state(NumberType::SLEEP_MODE_MIN, v)) sleep_min = (uint16_t)v; }
        // QC cluster (Stage 4). Optimistic publish in LevoitSwitch / LevoitNumber
        // means the entity reflects the new value by the time we read it here.
        { bool b; if (self->try_get_switch_state(SwitchType::QUICK_CLEAN, b)) qc_enabled = b ? 1 : 0; }
        { float v; if (self->try_get_number_state(NumberType::QUICK_CLEAN_MIN, v)) qc_min = (uint16_t)v; }
        { float v; if (self->try_get_number_state(NumberType::QUICK_CLEAN_FAN_LEVEL, v)) qc_fan = (uint8_t)v; }
        // DT cluster (Stage 5).
        { bool b; if (self->try_get_switch_state(SwitchType::DAYTIME_ENABLED, b)) dt_enabled = b ? 1 : 0; }
        { size_t i; if (self->try_get_select_index(SelectType::DAYTIME_FAN_MODE, i)) dt_mode = (uint8_t)i; }
        { float v; if (self->try_get_number_state(NumberType::DAYTIME_FAN_LEVEL, v)) dt_level = (uint8_t)v; }
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

      case CommandType::setFilterPercent:
      {
        // EverestAir: push computed filter % to the MCU so the panel indicator
        // matches. CMD=02 05 55, PAY=02 01 <pct>. Discovered from a startup
        // capture sending `02 05 55 02 01 64` (=100%). Value is 0–100.
        msg_type = {0x02, 0x05, 0x55};
        uint8_t pct = self->get_pending_filter_pct();
        if (pct > 100) pct = 100;
        ESP_LOGD(TAG_VITAL_CMD, "setFilterPercent: %u%%", pct);
        payload = {0x02, 0x01, pct};
        break;
      }

      case CommandType::setVentAngle:
      {
        // EverestAir vent louver angle. CMD=02 12 55, PAY=01 01 <deg>.
        // Status echoes the angle back in TLV tag 0x14. Hardware range 45–90°.
        msg_type = {0x02, 0x12, 0x55};
        uint8_t angle = 75;  // default louver position on power-up
        float v_angle;
        if (self->try_get_number_state(NumberType::VENT_ANGLE, v_angle))
          angle = static_cast<uint8_t>(v_angle);
        if (angle < 45) angle = 45;
        if (angle > 90) angle = 90;
        ESP_LOGD(TAG_VITAL_CMD, "setVentAngle: %u deg", angle);
        payload = {0x01, 0x01, angle};
        break;
      }

      // Eleva 300X: panel beeps. The value is a 4-byte LE field even though
      // only 0/1 is used (status echoes it in TLV 0x2D).
      case CommandType::setMuteOn:
        msg_type = {0x02, 0x17, 0x55};
        payload = {0x01, 0x04, 0x01, 0x00, 0x00, 0x00};
        break;

      case CommandType::setMuteOff:
        msg_type = {0x02, 0x17, 0x55};
        payload = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00};
        break;

      // Eleva 300X: AQI light ring brightness. Same CMD as the display
      // (02 04 55) but key 0x02; 0 = bright, 1 = dim (status TLV 0x32).
      case CommandType::setAqiRingBright:
        msg_type = {0x02, 0x04, 0x55};
        payload = {0x02, 0x01, 0x00};
        break;

      case CommandType::setAqiRingDim:
        msg_type = {0x02, 0x04, 0x55};
        payload = {0x02, 0x01, 0x01};
        break;

      // Eleva 300X auto-mode config (02 21 55). The stock ESP writes the quiet
      // settings as one group 05 06 07 08 and adaptive start as 01 02 03 04,
      // so both builders send their whole group: current entity state where
      // there is one, otherwise the value last read from the MCU.
      case CommandType::setElevaQuietConfig:
      {
        const ElevaAutoConfig &c = self->eleva_auto_config();
        bool quiet_on = c.quiet_enabled == 1;
        self->try_get_switch_state(SwitchType::QUIET_PERIOD, quiet_on);
        size_t trigger = c.quiet_trigger;  // 0 = light, 1 = timer
        self->try_get_select_index(SelectType::QUIET_TRIGGER, trigger);
        if (trigger > 1) trigger = 0;
        uint32_t start_s = c.quiet_start, end_s = c.quiet_end;
        float h;
        if (self->try_get_number_state(NumberType::QUIET_START, h))
          start_s = static_cast<uint32_t>(h * 3600.0f + 0.5f);
        if (self->try_get_number_state(NumberType::QUIET_END, h))
          end_s = static_cast<uint32_t>(h * 3600.0f + 0.5f);
        if (start_s > 86399) start_s = 86399;
        if (end_s > 86399) end_s = 86399;
        ESP_LOGD(TAG_VITAL_CMD, "setElevaQuietConfig: quiet=%u trigger=%s window=%u-%u s",
                 quiet_on ? 1u : 0u, trigger == 1 ? "timer" : "light", (unsigned)start_s, (unsigned)end_s);
        msg_type = {0x02, 0x21, 0x55};
        payload = {0x05, 0x01, static_cast<uint8_t>(quiet_on ? 1 : 0),
                   0x06, 0x01, static_cast<uint8_t>(trigger),
                   0x07, 0x04, (uint8_t)(start_s & 0xFF), (uint8_t)((start_s >> 8) & 0xFF),
                               (uint8_t)((start_s >> 16) & 0xFF), (uint8_t)((start_s >> 24) & 0xFF),
                   0x08, 0x04, (uint8_t)(end_s & 0xFF), (uint8_t)((end_s >> 8) & 0xFF),
                               (uint8_t)((end_s >> 16) & 0xFF), (uint8_t)((end_s >> 24) & 0xFF)};
        break;
      }

      case CommandType::setElevaAutoFlags:
      {
        const ElevaAutoConfig &c = self->eleva_auto_config();
        if (!c.known)
        {
          // Key 01 (meaning TBD) is echoed from the MCU's block; guessing it
          // could change behaviour, so wait for the block and ask for it.
          ESP_LOGW(TAG_VITAL_CMD, "setElevaAutoFlags: auto config not read yet, requesting it first");
          msg_type = {0x02, 0x21, 0x55};
          payload = {0x0C, 0x01, 0x00};
          break;
        }
        bool adaptive = c.adaptive_start == 1;
        bool dyn_manual = c.dynamic_manual == 1;
        bool dyn_sleep = c.dynamic_sleep == 1;
        self->try_get_switch_state(SwitchType::ADAPTIVE_START, adaptive);
        self->try_get_switch_state(SwitchType::DYNAMIC_MANUAL, dyn_manual);
        self->try_get_switch_state(SwitchType::DYNAMIC_SLEEP, dyn_sleep);
        ESP_LOGD(TAG_VITAL_CMD, "setElevaAutoFlags: adaptive=%u dyn_manual=%u dyn_sleep=%u",
                 adaptive ? 1u : 0u, dyn_manual ? 1u : 0u, dyn_sleep ? 1u : 0u);
        msg_type = {0x02, 0x21, 0x55};
        payload = {0x01, 0x01, c.profile,
                   0x02, 0x01, static_cast<uint8_t>(adaptive ? 1 : 0),
                   0x03, 0x01, static_cast<uint8_t>(dyn_manual ? 1 : 0),
                   0x04, 0x01, static_cast<uint8_t>(dyn_sleep ? 1 : 0)};
        break;
      }

      case CommandType::resetFilter:
        // Eleva 300X filter reset, as the stock app sends it: CMD 02 05 55,
        // key 03 with an empty value, followed by setElevaFilterNotGenuine
        // (04 01 00) - the app only resets non-genuine filters, so that pair is
        // the only reset sequence captured. The panel reset arrives the other
        // way as 02 05 55 + 03 01 00 (see decoder).
        if (self->get_model() != ModelType::ELEVA300X)
        {
          ESP_LOGW(TAG_VITAL_CMD, "resetFilter: no MCU filter reset known for this model");
          return {};
        }
        msg_type = {0x02, 0x05, 0x55};
        payload = {0x03, 0x00};
        break;

      case CommandType::setElevaFilterNotGenuine:
        // Key 04 = genuine filter linked in the VeSync app. ESPHome cannot
        // verify that, and the flag has no visible effect on the unit.
        msg_type = {0x02, 0x05, 0x55};
        payload = {0x04, 0x01, 0x00};
        break;

      case CommandType::requestElevaAutoConfig:
        msg_type = {0x02, 0x21, 0x55};
        payload = {0x0C, 0x01, 0x00};
        break;

      case CommandType::setElevaLocalTime:
      {
        const uint32_t t = self->get_pending_local_time();
        msg_type = {0x02, 0x22, 0x55};
        payload = {0x01, 0x04, (uint8_t)(t & 0xFF), (uint8_t)((t >> 8) & 0xFF),
                   (uint8_t)((t >> 16) & 0xFF), (uint8_t)((t >> 24) & 0xFF)};
        break;
      }

      default:
        ESP_LOGW(TAG_VITAL_CMD, "Command not implemented: %s", command_type_to_string(cmd));
        return {};
      }

      return build_levoit_message(msg_type, payload, messageUpCounter);
    }

  } // namespace levoit
} // namespace esphome
