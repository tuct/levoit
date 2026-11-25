#include "decoder.h"
#include "vital_status.h"
#include "core_status.h"
#include "sprout_status.h"
#include "superior_status.h"
#include "esphome/core/log.h"
#include "types.h"

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG_DEC = "levoit.decoder";
    struct PayloadHashCacheEntry
    {
      uint8_t model; // ModelType
      uint8_t ptype0;
      uint8_t ptype1;
      uint8_t hash; // fingerprint of payload
      bool valid;
    };

    static PayloadHashCacheEntry g_payload_hash_cache[16];

    static uint8_t compute_payload_hash_(const uint8_t *payload, size_t len)
    {
      uint8_t h = 0;
      for (size_t i = 0; i < len; i++)
      {
        h ^= payload[i];
        h = (h << 1) | (h >> 7); // rotate-left 1
      }
      return h;
    }

    static bool payload_changed_(ModelType model,
                                 uint8_t ptype0,
                                 uint8_t ptype1,
                                 const uint8_t *payload,
                                 size_t payload_len)
    {
      if (!payload || payload_len == 0)
        return true;

      const uint8_t hash = compute_payload_hash_(payload, payload_len);

      // Find existing entry
      for (auto &e : g_payload_hash_cache)
      {
        if (e.valid &&
            e.model == (uint8_t)model &&
            e.ptype0 == ptype0 &&
            e.ptype1 == ptype1)
        {

          if (e.hash == hash)
          {
            return false; // identical payload → skip
          }

          e.hash = hash;
          return true; // changed → decode
        }
      }

      // New entry
      for (auto &e : g_payload_hash_cache)
      {
        if (!e.valid)
        {
          e.valid = true;
          e.model = (uint8_t)model;
          e.ptype0 = ptype0;
          e.ptype1 = ptype1;
          e.hash = hash;
          return true;
        }
      }

      // Cache full → always decode
      return true;
    }
    static bool is_status_push_(ModelType model, uint8_t msg_type, uint8_t ptype0, uint8_t ptype1)
    {
      if (model == ModelType::ELEVA300X && msg_type == 0x02)
        return ptype0 == 0x00 && ptype1 == 0x55;
      if (msg_type != 0x22)
        return false;
      return (ptype0 == 0x00 && ptype1 == 0x55) || // Vital / Sprout / EverestAir / Eleva
             (ptype0 == 0x30 && ptype1 == 0x55) || // Superior 6000S
             (ptype0 == 0x30 && ptype1 == 0x40) || // Core 300S / 400S
             (ptype0 == 0xB0 && ptype1 == 0x40) || // Core 300S / 400S alt
             (ptype0 == 0x32 && ptype1 == 0x40) || // Core 300S, 19-byte status
             (ptype0 == 0x60 && ptype1 == 0x40) || // Core 200S
             (ptype0 == 0x40 && ptype1 == 0x41);   // Core 600S
    }

    void dispatch_decoder(Levoit *self,
                          ModelType model,
                          uint8_t msg_type,
                          uint8_t seq,
                          uint8_t cmd_size_code,
                          uint8_t ptype0,
                          uint8_t ptype1,
                          const uint8_t *payload,
                          size_t payload_len)
    {
      (void)msg_type;
      (void)seq;
      (void)cmd_size_code;

      ESP_LOGD(TAG_DEC, "dispatch: model=%d ptype=%02X%02X payload_len=%u",
               (int)model, ptype0, ptype1, (unsigned)payload_len);
      // ack all messages!
      //only if (0x22)
      // Eleva 300X: the MCU pushes status (02 00 55), sensor (02 23 55) and
      // auto-config (02 21 55) frames as REPORT (0x02); the stock ESP ACKs those too.
      if (msg_type == 0x22 || (model == ModelType::ELEVA300X && msg_type == 0x02))
      {
        if (model == ModelType::SUPERIOR6000S && ptype0 == 0x44 && ptype1 == 0x55)
          self->ackFilterReset(ptype0, ptype1);
        else
          self->ackMessage(ptype0, ptype1, seq);
      }

      // Eleva 300X: echo every sensor REPORT back, like the stock firmware.
      // Ahead of the dedup gate: identical readings must still be mirrored.
      if (model == ModelType::ELEVA300X && msg_type == 0x02 && ptype0 == 0x23 && ptype1 == 0x55)
      {
        self->mirror_eleva_sensor_report(payload, payload_len);
      }

      // Eleva 300X: filter reset on the panel. The MCU sends SEND 02 05 55 with
      // key 03 (captured: 03 01 00); filter life lives on the ESP, so reset
      // our counters. Ahead of the dedup gate: two panel resets in a row carry
      // the same payload.
      if (model == ModelType::ELEVA300X && msg_type == 0x22 && ptype0 == 0x05 && ptype1 == 0x55 &&
          payload_len >= 2 && payload[0] == 0x03)
      {
        ESP_LOGI(TAG_DEC, "Filter reset from the unit's panel (Eleva 300X)");
        self->set_used_cadr(0);
        self->set_total_runtime(0);
        self->publish_filter_stats_now();
      }

      // Eleva 300X auto-mode config block: in the MCU's ACK to our 0C query
      // and as a REPORT after every change (app, panel or our own write).
      if (model == ModelType::ELEVA300X && (msg_type == 0x02 || msg_type == 0x12) &&
          ptype0 == 0x21 && ptype1 == 0x55)
      {
        self->apply_eleva_auto_config(payload, payload_len);
      }

      // Eleva 300X: MCU asks for the local time at boot (SEND 02 22 55, no
      // payload). Already ACKed above; answer with the time if we have it.
      if (model == ModelType::ELEVA300X && msg_type == 0x22 && ptype0 == 0x22 && ptype1 == 0x55)
      {
        self->send_eleva_local_time();
      }

      // Core200S / Core300S: the MCU pushes 01 E4 A5 when the filter is reset
      // with the button on the unit itself (captured in
      // devices/levoit-core200s/uart/filter_from_device.txt; on a Core 300S,
      // holding Sleep for 3 s sends the same frame with payload 0x00). The same
      // message travels the other way with payload 0x00 as the reset command.
      //
      // The payload is NOT a reliable marker: MCU 2.0.11 pushes 0x01, while
      // 4.0.34 on the Core 200S Plus pushes 0x00 (issue #60). Any inbound 0x22
      // of this type is MCU-originated - our own reset goes out as 0x22 and
      // comes back as a 0x12 response - so the direction alone identifies it.
      //
      // Handled ahead of the dedup gate below, because two panel resets in a
      // row carry an identical payload and the second would otherwise be
      // skipped.
      if ((model == ModelType::CORE200S || model == ModelType::CORE300S) &&
          msg_type == 0x22 && ptype0 == 0xE4 && ptype1 == 0xA5)
      {
        ESP_LOGI(TAG_DEC, "Filter reset from the unit's button");
        self->set_used_cadr(0);
        self->set_total_runtime(0);
        self->publish_filter_stats_now();
      }

      uint8_t h = compute_payload_hash_(payload, payload_len);
      ESP_LOGD("levoit.dedup", "model=%d ptype=%02X%02X payload_len=%u hash=0x%02X last=0x%02X",
               (int)model, ptype0, ptype1, (unsigned)payload_len, h,
               payload_len ? payload[payload_len - 1] : 0);

      // Deduplicate only periodic status pushes; timer responses, button resets,
      // and async event notifications must always be processed even when two
      // consecutive frames carry identical payloads.
      if (is_status_push_(model, msg_type, ptype0, ptype1) &&
          !payload_changed_(model, ptype0, ptype1, payload, payload_len))
      {
        ESP_LOGV(TAG_DEC,
                 "skip decode (unchanged payload): model=%d ptype=%02X%02X",
                 (int)model, ptype0, ptype1);
      }
      else
      {

        if (model == ModelType::CORE300S || model == ModelType::CORE400S)
        {
          // Core300S/400S status: ptype 0x30 0x40 or 0xB0 0x40
          if (msg_type == 0x22 && ((ptype0 == 0x30 && ptype1 == 0x40) || (ptype0 == 0xB0 && ptype1 == 0x40)))
          {
            decode_core_status(self, model, payload, payload_len);
          }
          // Core300S 19-byte status: ptype 0x32 0x40. Seen on a LAP-C302S-WGC
          // whose MCU reports 1.0.0; it never sends 0x30 0x40 or 0xB0 0x40.
          if (model == ModelType::CORE300S && msg_type == 0x22 && ptype0 == 0x32 && ptype1 == 0x40)
          {
            decode_core300s_status_19(self, payload, payload_len);
          }
          // The same status, returned in the ACK to requestCore300sStatus (01 33 40).
          if (model == ModelType::CORE300S && msg_type == 0x12 && ptype0 == 0x33 && ptype1 == 0x40)
          {
            decode_core300s_status_19(self, payload, payload_len);
          }
          // Timer
          if (msg_type == 0x12 && ptype0 == 0x65 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
          if (msg_type == 0x22 && ptype0 == 0x66 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
        }
        // On/Off button hold (all Core models): the MCU sends 01 02 D1 with 02 once
        // the hold registers, a few seconds in, and 03 on release. Stock firmware
        // starts Wi-Fi setup on it ("netcfg is start"); per the Core 300S manual
        // a 15 s hold disconnects Wi-Fi and restores defaults. Seen on a Core 300S.
        if ((model == ModelType::CORE200S || model == ModelType::CORE300S ||
             model == ModelType::CORE400S || model == ModelType::CORE600S) &&
            msg_type == 0x22 && ptype0 == 0x02 && ptype1 == 0xD1 && payload_len >= 1)
        {
          if (payload[0] == 0x02)
            self->publish_binary_sensor(BinarySensorType::POWER_BUTTON_HELD, true);
          else if (payload[0] == 0x03)
            self->publish_binary_sensor(BinarySensorType::POWER_BUTTON_HELD, false);
        }
        if (model == ModelType::CORE200S)
        {
          // Core200S status: frame bytes {0x01, 0x60, 0x40} → ptype0=0x60, ptype1=0x40
          if (msg_type == 0x22 && ptype0 == 0x60 && ptype1 == 0x40)
          {
            decode_core_status(self, model, payload, payload_len);
          }
          // Timer: device echoes set command back (ptype 0x64) or pushes update (ptype 0x66)
          if (msg_type == 0x22 && ptype0 == 0x64 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
          if (msg_type == 0x12 && ptype0 == 0x65 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
          if (msg_type == 0x22 && ptype0 == 0x66 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
        }
        if (model == ModelType::CORE600S)
        {
          // Core600S status push: MCU sends SEND(0x22) CMD=01 40 41
          if (msg_type == 0x22 && ptype0 == 0x40 && ptype1 == 0x41)
          {
            decode_core_status(self, model, payload, payload_len);
          }
          // Timer
          if (msg_type == 0x12 && ptype0 == 0x65 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
          if (msg_type == 0x22 && ptype0 == 0x66 && ptype1 == 0xA2)
          {
            decode_core_timer(self, model, payload, payload_len);
          }
        }
        if (model == ModelType::VITAL100S || model == ModelType::VITAL200S || model == ModelType::SPROUT || model == ModelType::EVERESTAIR || model == ModelType::ELEVA300X)
        {
          // Vital/Sprout/EverestAir/Eleva models: status payload is flat TLV, CMD=02 00 55.
          // The Eleva 300X sends it as REPORT (0x02) instead of SEND (0x22).
          const bool status_type_ok = msg_type == 0x22 || (model == ModelType::ELEVA300X && msg_type == 0x02);
          if (status_type_ok && ptype0 == 0x00 && ptype1 == 0x55)
          {
            decode_vital_status(self, model, payload, payload_len);
          }
          // Timer ack payload, sent after status requested
          if (msg_type == 0x12 && ptype0 == 0x1A && ptype1 == 0x50)
          {
            decode_vital_timer(self, model, payload, payload_len);
          }
          // Timer update pushed from device (Eleva 300X: as REPORT, every second)
          if (status_type_ok && ptype0 == 0x1B && ptype1 == 0x50)
          {
            decode_vital_timer(self, model, payload, payload_len);
          }
          // Sprout: async MCU event notifications (CMD=02 08 55)
          if (model == ModelType::SPROUT && msg_type == 0x22 && ptype0 == 0x08 && ptype1 == 0x55)
          {
            decode_sprout_event(self, payload, payload_len);
          }
        }
        if (model == ModelType::SUPERIOR6000S)
        {
          // Superior 6000S: status report is a TLV payload under CMD=02 30 55
          if (msg_type == 0x22 && ptype0 == 0x30 && ptype1 == 0x55)
          {
            decode_superior_status(self, model, payload, payload_len);
          }
          // Timer set from the device's own button (same shape as the Vital timer)
          if (msg_type == 0x22 && ptype0 == 0x1B && ptype1 == 0x50)
          {
            decode_superior_timer(self, model, payload, payload_len);
          }
          // Filter reset pressed on the display
          if (msg_type == 0x22 && ptype0 == 0x44 && ptype1 == 0x55)
          {
            ESP_LOGI(TAG_DEC, "Filter reset from display (Superior 6000S)");
            self->set_used_cadr(0);
            self->set_total_runtime(0);
            self->publish_filter_stats_now();
          }
        }
      }


      // Other models: add other decoders later
    }

  } // namespace levoit
} // namespace esphome
