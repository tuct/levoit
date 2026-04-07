#include "sprout_status.h"
#include "tlv.h"
#include "types.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>

namespace esphome
{
  namespace levoit
  {
    static const char *const TAG_SPROUT = "levoit.sprout";

    // Decode CMD=02 08 55 — async event notifications pushed by the Sprout MCU.
    //
    // Observed payload patterns (flat TLV, same format as vital status):
    //   tag=01 len=01 val=01/00  →  button press / toggle event
    //   tag=03 len=03 val=01 {LE16}  →  PM threshold crossed, LE16 = PM raw value
    //   tag=04 len=01 val=00/01  →  cover sensor (0=closed, 1=open)
    void decode_sprout_event(Levoit *self,
                             const uint8_t *payload,
                             size_t payload_len)
    {
      if (!payload || payload_len == 0)
        return;

      std::vector<LevoitTLV> tlvs;
      bool ok = extract_tlvs_from_payload(ModelType::SPROUT, payload, payload_len, tlvs, 0);
      if (!ok)
      {
        ESP_LOGW(TAG_SPROUT, "TLV extraction failed for sprout event (len=%u)", (unsigned)payload_len);
        return;
      }

      for (const auto &t : tlvs)
      {
        switch (t.tag)
        {
        case 0x01:
          ESP_LOGD(TAG_SPROUT, "Event: button/toggle val=%u", (unsigned)t.value_u32);
          if (self != nullptr)
          {
            std::string event_str = (t.value_u32 == 1) ? "button_press" : "button_release";
            self->publish_text_sensor(TextSensorType::SPROUT_EVENT, event_str);
          }
          break;

        case 0x03:
          // White noise state change: {active(0=stopped,1=running), volume(0-255), sound_index(0-14)}
          // Observed: 03 03 00 F8 04  →  stopped, vol=248, sound=4
          if (t.value_len >= 3 && t.value_ptr != nullptr)
          {
            uint8_t wn_active = t.value_ptr[0];
            uint8_t wn_vol    = t.value_ptr[1];
            uint8_t wn_sound  = t.value_ptr[2];
            ESP_LOGD(TAG_SPROUT, "Event: WN state active=%u vol=%u sound=%u", wn_active, wn_vol, wn_sound);
            if (self != nullptr)
            {
              self->publish_switch(SwitchType::WHITE_NOISE, wn_active == 1);
              self->publish_number(NumberType::WHITE_NOISE_VOLUME, wn_vol);
              self->publish_select(SelectType::WHITE_NOISE_SOUND, wn_sound);
            }
          }
          break;

        case 0x04:
          ESP_LOGD(TAG_SPROUT, "Event: cover sensor val=%u", (unsigned)t.value_u32);
          if (self != nullptr)
            self->publish_binary_sensor(BinarySensorType::COVER_OPEN, t.value_u32 == 1);
          break;

        default:
          ESP_LOGV(TAG_SPROUT, "Event: unknown tag=0x%02X len=%u val=%u",
                   t.tag, t.value_len, (unsigned)t.value_u32);
          break;
        }
      }
    }

  } // namespace levoit
} // namespace esphome
