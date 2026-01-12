#include "decoder.h"
#include "vital_status.h"
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

      ESP_LOGV(TAG_DEC, "dispatch: model=%d ptype=%02X%02X payload_len=%u",
               (int)model, ptype0, ptype1, (unsigned)payload_len);

      uint8_t h = compute_payload_hash_(payload, payload_len);
      ESP_LOGV("levoit.dedup", "model=%d ptype=%02X%02X payload_len=%u hash=0x%02X last=0x%02X",
               (int)model, ptype0, ptype1, (unsigned)payload_len, h,
               payload_len ? payload[payload_len - 1] : 0);

      // only if payload changed
      if (!payload_changed_(model, ptype0, ptype1, payload, payload_len))
      {
        ESP_LOGV(TAG_DEC,
                 "skip decode (unchanged payload): model=%d ptype=%02X%02X",
                 (int)model, ptype0, ptype1);
      }
      else
      {
        // Vital models: payload is TLV 0x00 0x55
        if (msg_type == 0x22 && ptype0 == 0x00 && ptype1 == 0x55 && (model == ModelType::VITAL100S || model == ModelType::VITAL200S))
        {
          decode_vital_status(self, model, payload, payload_len);
        }
        // Vital models: timer ack payload, send after status requested
        if (msg_type == 0x12 && ptype0 == 0x1A && ptype1 == 0x50 && (model == ModelType::VITAL100S || model == ModelType::VITAL200S))
        {
          decode_vital_timer(self, model, payload, payload_len);
        }
        // Vital models: timer updated from device msg
        if (msg_type == 0x22 && ptype0 == 0x1B && ptype1 == 0x50 && (model == ModelType::VITAL100S || model == ModelType::VITAL200S))
        {
          decode_vital_timer(self, model, payload, payload_len);
        }
      }

      // ack all messages!
      self->ackMessage(ptype0, ptype1);
      // Other models: add other decoders later
    }

  } // namespace levoit
} // namespace esphome
