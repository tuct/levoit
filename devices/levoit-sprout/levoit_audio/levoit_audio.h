#pragma once

// ---------------------------------------------------------------------------
// Levoit Sprout — LittleFS audio playback over I2S
//
// Decodes MP3 and OGG files stored in the "assets" LittleFS partition and
// streams PCM to the I2S amplifier (NS4168 / MAX98357A).
//
// Decoders (single-header, no external libs):
//   MP3 — dr_mp3   https://github.com/mackron/dr_libs
//   OGG — stb_vorbis  https://github.com/nothings/stb
//
// Place dr_mp3.h and stb_vorbis.c next to this file.
// ---------------------------------------------------------------------------

#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "driver/i2s.h"
#include "esp_littlefs.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Single-header decoders — define implementation exactly once
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO          // use our own file I/O
#include "dr_mp3.h"

#define STB_VORBIS_NO_STDIO      // use memory buffer
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"

static const char *const TAG_AUDIO = "levoit.audio";

// ---------------------------------------------------------------------------
// Pin definitions (confirmed by logic analyzer)
// ---------------------------------------------------------------------------
#define SPROUT_I2S_NUM        I2S_NUM_0
#define SPROUT_I2S_BCLK       GPIO_NUM_5
#define SPROUT_I2S_LRCLK      GPIO_NUM_18
#define SPROUT_I2S_DOUT       GPIO_NUM_17
#define SPROUT_AMP_ENABLE     GPIO_NUM_19

#define SPROUT_SAMPLE_RATE    32000
#define SPROUT_BITS           16
#define SPROUT_CHANNELS       1      // mono amp, duplicate L+R if needed
#define SPROUT_DMA_BUF_COUNT  8
#define SPROUT_DMA_BUF_LEN    512

// ---------------------------------------------------------------------------
// LittleFS mount
// ---------------------------------------------------------------------------
static bool littlefs_mounted_ = false;

static bool mount_littlefs() {
    if (littlefs_mounted_) return true;

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/assets",
        .partition_label = "assets",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_AUDIO, "LittleFS mount failed: %s", esp_err_to_name(err));
        return false;
    }
    littlefs_mounted_ = true;
    ESP_LOGI(TAG_AUDIO, "LittleFS mounted at /assets");
    return true;
}

// ---------------------------------------------------------------------------
// I2S init / deinit
// ---------------------------------------------------------------------------
static bool i2s_running_ = false;
static volatile bool stop_requested_ = false;
static TaskHandle_t audio_task_ = nullptr;

static void i2s_start() {
    if (i2s_running_) return;

    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SPROUT_SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,  // stereo frame, amp uses one channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = SPROUT_DMA_BUF_COUNT,
        .dma_buf_len          = SPROUT_DMA_BUF_LEN,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = SPROUT_I2S_BCLK,
        .ws_io_num    = SPROUT_I2S_LRCLK,
        .data_out_num = SPROUT_I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };
    i2s_driver_install(SPROUT_I2S_NUM, &cfg, 0, nullptr);
    i2s_set_pin(SPROUT_I2S_NUM, &pins);
    i2s_zero_dma_buffer(SPROUT_I2S_NUM);
    i2s_running_ = true;

    // Enable amplifier
    gpio_set_direction(SPROUT_AMP_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(SPROUT_AMP_ENABLE, 1);
    ESP_LOGI(TAG_AUDIO, "I2S started, amp enabled");
}

static void i2s_stop() {
    if (!i2s_running_) return;
    i2s_zero_dma_buffer(SPROUT_I2S_NUM);
    gpio_set_level(SPROUT_AMP_ENABLE, 0);   // mute amp
    i2s_driver_uninstall(SPROUT_I2S_NUM);
    i2s_running_ = false;
    ESP_LOGI(TAG_AUDIO, "I2S stopped, amp muted");
}

// Write 16-bit mono samples as stereo (duplicate to L+R)
static void write_samples(const int16_t *samples, size_t count) {
    // Interleave mono → stereo: [L, R, L, R, ...]
    static int16_t stereo_buf[SPROUT_DMA_BUF_LEN * 2];
    while (count > 0) {
        size_t chunk = (count > SPROUT_DMA_BUF_LEN) ? SPROUT_DMA_BUF_LEN : count;
        const int32_t vol = audio_volume_;
        for (size_t i = 0; i < chunk; i++) {
            int16_t s = (int16_t)((int32_t)samples[i] * vol / 255);
            stereo_buf[i * 2]     = s;
            stereo_buf[i * 2 + 1] = s;
        }
        size_t written = 0;
        i2s_write(SPROUT_I2S_NUM,
                  stereo_buf,
                  chunk * 2 * sizeof(int16_t),
                  &written,
                  portMAX_DELAY);
        samples += chunk;
        count -= chunk;
    }
}

// ---------------------------------------------------------------------------
// MP3 playback (dr_mp3, file-based)
// ---------------------------------------------------------------------------
static bool play_mp3(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG_AUDIO, "Cannot open %s", path);
        return false;
    }

    // Read whole file into heap (files are small, ~200–400 KB)
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(file_size);
    if (!buf) {
        ESP_LOGE(TAG_AUDIO, "OOM for %s (%u bytes)", path, (unsigned)file_size);
        fclose(f);
        return false;
    }
    fread(buf, 1, file_size, f);
    fclose(f);

    dr_mp3 mp3;
    if (!drmp3_init_memory(&mp3, buf, file_size, nullptr)) {
        ESP_LOGE(TAG_AUDIO, "drmp3_init_memory failed");
        free(buf);
        return false;
    }

    ESP_LOGI(TAG_AUDIO, "Playing MP3: %s  %u Hz  %u ch", path,
             mp3.sampleRate, mp3.channels);

    static drmp3_int16 pcm[SPROUT_DMA_BUF_LEN];
    drmp3_uint64 frames_read;
    while (!stop_requested_ &&
           (frames_read = drmp3_read_pcm_frames_s16(&mp3, SPROUT_DMA_BUF_LEN, pcm)) > 0) {
        if (mp3.channels == 2) {
            for (drmp3_uint64 i = 0; i < frames_read; i++)
                pcm[i] = (int16_t)(((int32_t)pcm[i * 2] + pcm[i * 2 + 1]) / 2);
        }
        write_samples(pcm, (size_t)frames_read);
    }

    drmp3_uninit(&mp3);
    free(buf);
    ESP_LOGI(TAG_AUDIO, "MP3 done: %s", path);
    return true;
}

// ---------------------------------------------------------------------------
// OGG/Vorbis playback (stb_vorbis, memory-based)
// ---------------------------------------------------------------------------
static bool play_ogg(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG_AUDIO, "Cannot open %s", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(file_size);
    if (!buf) {
        ESP_LOGE(TAG_AUDIO, "OOM for %s (%u bytes)", path, (unsigned)file_size);
        fclose(f);
        return false;
    }
    fread(buf, 1, file_size, f);
    fclose(f);

    int error = 0;
    stb_vorbis *v = stb_vorbis_open_memory(buf, (int)file_size, &error, nullptr);
    if (!v) {
        ESP_LOGE(TAG_AUDIO, "stb_vorbis_open failed: error %d", error);
        free(buf);
        return false;
    }

    stb_vorbis_info info = stb_vorbis_get_info(v);
    ESP_LOGI(TAG_AUDIO, "Playing OGG: %s  %u Hz  %u ch", path,
             info.sample_rate, info.channels);

    static int16_t pcm[SPROUT_DMA_BUF_LEN];
    int samples;
    while (!stop_requested_ &&
           (samples = stb_vorbis_get_samples_short_interleaved(
                v, info.channels, pcm, SPROUT_DMA_BUF_LEN * info.channels)) > 0) {
        if (info.channels == 2) {
            for (int i = 0; i < samples; i++)
                pcm[i] = (int16_t)(((int32_t)pcm[i * 2] + pcm[i * 2 + 1]) / 2);
        }
        write_samples(pcm, (size_t)samples);
    }

    stb_vorbis_close(v);
    free(buf);
    ESP_LOGI(TAG_AUDIO, "OGG done: %s", path);
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Sound index 0–14 maps to filenames stored in LittleFS
// Adjust the list to match what is actually in your assets partition
// Confirmed pre-loaded files (from firmware flash dump).
// Sounds 2–5, 7–8, 10–11, 13–15 must be downloaded via the original app
// before flashing — see Sprout README for download procedure.
struct SoundDef {
    const char *file;
    const char *name;
};

// Confirmed pre-loaded files (from firmware flash dump).
// Sounds marked "(app)" must be downloaded via the Levoit app before flashing.
static const SoundDef SOUNDS[] = {
    { "/assets/100001.ogg",  "Summer Rain" },           //  0 pre-loaded
    { "/assets/100002.mp3",  "Gentle Sea Wave" },       //  1 pre-loaded
    { "/assets/100003.mp3",  "Sound 03" },              //  2 app
    { "/assets/100004.mp3",  "Sound 04" },              //  3 app
    { "/assets/100005.mp3",  "Sound 05" },              //  4 app
    { "/assets/100006.mp3",  "Insects by Stream" },     //  5 pre-loaded
    { "/assets/100007.mp3",  "Sound 07" },              //  6 app
    { "/assets/100008.mp3",  "Sound 08" },              //  7 app
    { "/assets/100009.mp3",  "Morning Seaside" },       //  8 pre-loaded
    { "/assets/1000010.mp3", "Sound 10" },              //  9 app
    { "/assets/1000011.mp3", "Sound 11" },              // 10 app
    { "/assets/1000012.mp3", "Sound 12" },              // 11 pre-loaded
    { "/assets/1000013.mp3", "Sound 13" },              // 12 app
    { "/assets/1000014.mp3", "Sound 14" },              // 13 app
    { "/assets/1000015.mp3", "Sound 15" },              // 14 app
};
static constexpr size_t SOUND_COUNT = sizeof(SOUNDS) / sizeof(SOUNDS[0]);

// Backwards-compatible alias
static inline const char *SOUND_FILES_compat(size_t i) { return SOUNDS[i].file; }

static volatile bool loop_enabled_ = false;
static volatile uint8_t audio_volume_ = 200;  // 0–255, default ~80%

// ---------------------------------------------------------------------------
// FreeRTOS task — runs decode loop, deletes itself when done
// ---------------------------------------------------------------------------
struct AudioTaskParams {
    uint8_t index;
};

static void audio_task_fn(void *arg) {
    AudioTaskParams *p = (AudioTaskParams *)arg;
    uint8_t index = p->index;
    delete p;

    i2s_start();

    do {
        const char *path = SOUNDS[index].file;
        if (strstr(path, ".ogg"))
            play_ogg(path);
        else
            play_mp3(path);
    } while (loop_enabled_ && !stop_requested_);

    i2s_stop();
    audio_task_ = nullptr;
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Call once from ESPHome setup()
inline void levoit_audio_setup() {
    mount_littlefs();
    gpio_set_direction(SPROUT_AMP_ENABLE, GPIO_MODE_OUTPUT);
    gpio_set_level(SPROUT_AMP_ENABLE, 0);
}

// Play sound by index (0–14), optionally looping.
// Returns immediately — decoding runs on a background FreeRTOS task.
// Stops any currently playing sound first.
inline void levoit_audio_play(uint8_t index, bool loop = false) {
    if (index >= SOUND_COUNT) {
        ESP_LOGE(TAG_AUDIO, "Invalid sound index %u", index);
        return;
    }
    if (!mount_littlefs()) return;

    // Stop previous playback
    if (audio_task_ != nullptr) {
        loop_enabled_ = false;
        stop_requested_ = true;
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    stop_requested_ = false;
    loop_enabled_ = loop;

    ESP_LOGI(TAG_AUDIO, "Play: [%u] %s%s", index, SOUNDS[index].name, loop ? " (loop)" : "");

    auto *p = new AudioTaskParams{index};
    xTaskCreate(audio_task_fn,
                "levoit_audio",
                8192,
                p,
                5,
                &audio_task_);
}

// Stop playback — task exits at next decode chunk boundary (~16 ms)
inline void levoit_audio_stop() {
    loop_enabled_ = false;
    stop_requested_ = true;
}

// Enable/disable loop on currently playing sound
inline void levoit_audio_set_loop(bool loop) {
    loop_enabled_ = loop;
}

// Volume 0–255. Applied per-sample via integer PCM scaling.
inline void levoit_audio_set_volume(uint8_t vol) { audio_volume_ = vol; }
inline uint8_t levoit_audio_get_volume() { return audio_volume_; }

// Returns true while a sound is playing
inline bool levoit_audio_is_playing() {
    return audio_task_ != nullptr;
}

// Number of sounds
inline size_t levoit_audio_sound_count() {
    return SOUND_COUNT;
}

// Sound name for index (for ESPHome select options)
inline const char *levoit_audio_sound_name(size_t index) {
    return (index < SOUND_COUNT) ? SOUNDS[index].name : "";
}
