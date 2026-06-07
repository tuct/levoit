#include "levoit_select.h"
#include "../levoit.h"
#include "../types.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select_traits.h"

namespace esphome
{
  namespace levoit
  {

    static const char *const TAG = "levoit.select";

    void LevoitSelect::setup() {
      switch (this->type_) {
        case SelectType::AUTO_MODE:
          if (parent_ && parent_->get_model() == ModelType::CORE600S)
            this->traits.set_options({"Default", "Quiet", "Room Size", "ECO"});
          else
            this->traits.set_options({"Default", "Quiet", "Room Size"});
          break;
        case SelectType::SLEEP_MODE:
          this->traits.set_options({"Default","Custom"});
          break;
        // SelectType::QUICK_CLEAN_FAN_LEVEL case removed — see types.h note.
        case SelectType::WHITE_NOISE_FAN_LEVEL:
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});
          break;
        case SelectType::SLEEP_MODE_FAN_MODE_LEVEL:
          this->traits.set_options({"Low","Medium","High","Highest","Minimum"});
          break;
        case SelectType::DAYTIME_FAN_MODE:
          // Vital TLV 0x22: daytime preset fan-mode enum byte. Empirically
          // observed values: 0x00=Manual, 0x01=Sleep, 0x02=Auto, 0x05=Pet
          // (parallels the fan-mode byte at TLV 0x03). Bytes 0x03 and 0x04
          // were not exposed through the VeSync app captures we have; if
          // the MCU ever pushes those, label them Unknown3/Unknown4 in HA
          // so the publish_select index lookup doesn't fall outside the
          // options list (which would leave has_state()=false and the
          // entity stuck at "unknown" — see fix in 2f5d90b for the
          // analogous SLEEP_PREFERENCE case).
          // See docs/MCU_2.0.0_baseline.md TLV inventory.
          this->traits.set_options({"Manual", "Sleep", "Auto", "Unknown3", "Unknown4", "Pet"});
          break;
        case SelectType::NIGHTLIGHT:
          this->traits.set_options({"Off","Mid","Full"});
          break;
        case SelectType::LIGHT_MODE:
          this->traits.set_options({"Off", "Nightlight", "Breathing"});
          break;
        case SelectType::WHITE_NOISE_SOUND:
          this->traits.set_options({
              "Sound 01", "Sound 02", "Sound 03", "Sound 04", "Sound 05",
              "Sound 06", "Sound 07", "Sound 08", "Sound 09", "Sound 10",
              "Sound 11", "Sound 12", "Sound 13", "Sound 14", "Sound 15"});
          break;
        case SelectType::SLEEP_PREFERENCE:
          // TLV 0x18 byte values, empirically verified on MCU 2.0.0 via
          // on-device probing (Probe 3 + recovery steps): 0x00=Default,
          // 0x01=Custom1, 0x02=Custom2. The byte is also the "gate" for
          // the bulk-prefs SET path — when it's 0x00, writes to the other
          // 11 cluster TLVs (0x05..0x0F local tags on CMD 02 02 55) are
          // silently dropped by the MCU. See docs/STOCK_FIRMWARE_FINDINGS.md
          // ("Gate behavior") and docs/MCU_2.0.0_baseline.md ("Sleep / QC /
          // WN / DT bulk write") for the full protocol.
          this->traits.set_options({"Default", "Custom1", "Custom2"});
          break;
        default:
          break;
      }
    }
    void LevoitSelect::dump_config() { LOG_SELECT("", "Levoit Select", this); }

    void LevoitSelect::control(const std::string &value)
    {
      // Find index of the selected option
      const auto &options = this->traits.get_options();
      auto it = std::find(options.begin(), options.end(), value);

      if (it == options.end()) {
        ESP_LOGW(TAG, "Unknown select option: %s", value.c_str());
        return;
      }

      uint32_t index = it - options.begin();

      // Optimistic update for HA UI (string!)
      this->publish_state(value);

      if (!parent_) {
        ESP_LOGW(TAG, "No parent set for select");
        return;
      }

      // Send numeric command to device
      parent_->on_select_command(this->type_, index);
    }

  } // namespace levoit
} // namespace esphome
