#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_spiffs.h>
#include <esp_vfs.h>

#include <driver/i2s_std.h>
#include <audio_player.h>

#include "audio.h"

static const char *TAG = "audio";

static i2s_chan_handle_t i2s_tx_chan = NULL;

/* forward declarations */
static esp_err_t audio_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
static esp_err_t audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

/* -------------------------------------------------------------------------- */
/*                        I2S / hardware initialisation                       */
/* -------------------------------------------------------------------------- */

static esp_err_t audio_init(const i2s_std_config_t *i2s_config, i2s_chan_handle_t *tx_channel)
{
    /* If already initialised, just return the existing handle */
    if (i2s_tx_chan != NULL) {
        if (tx_channel) {
            *tx_channel = i2s_tx_chan;
        }
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = I2S_DUPLEX_STEREO_CFG(44100);
    const i2s_std_config_t *p_i2s_cfg = &std_cfg_default;
    if (i2s_config != NULL) {
        p_i2s_cfg = i2s_config;
    }

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_chan, p_i2s_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));

    if (tx_channel) {
        *tx_channel = i2s_tx_chan;
    }

    return ESP_OK;
}

/* -------------------------------------------------------------------------- */
/*                     Public helper to init "speaker" side                   */
/* -------------------------------------------------------------------------- */

static esp_err_t codec_init(void)
{
    /* Only config I2S once */
    return audio_init(NULL, &i2s_tx_chan);
}

/* -------------------------------------------------------------------------- */
/*                          Audio player front-end API                        */
/* -------------------------------------------------------------------------- */

esp_err_t app_audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    return audio_write(audio_buffer, len, bytes_written, timeout_ms);
}

esp_err_t audio_handle_info(PDM_SOUND_TYPE voice)
{
    char filepath[64];
    esp_err_t ret = ESP_OK;

    switch (voice) {
    case SOUND_TYPE_HIT_ME:
        sprintf(filepath, "%s/%s", CONFIG_SPIFFS_MOUNT_POINT, "HitMe.mp3");
        break;
    case SOUND_TYPE_GOOD_FOOT:
        sprintf(filepath, "%s/%s", CONFIG_SPIFFS_MOUNT_POINT, "GetOnGoodFoot.mp3");
        break;
    default:
        ESP_LOGE(TAG, "Unknown sound type: %d", voice);
        return ESP_FAIL;
    }

    FILE *fp = fopen(filepath, "r");
    ESP_GOTO_ON_FALSE(fp, ESP_FAIL, err, TAG, "Failed to open file: %s", filepath);

    ESP_LOGI(TAG, "play: %s", filepath);
    ret = audio_player_play(fp);

err:
    return ret;
}

/* -------------------------------------------------------------------------- */
/*                              Player callbacks                              */
/* -------------------------------------------------------------------------- */

static esp_err_t app_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    /* No external codec to mute; MAX98357A has no mute pin in this design */
    (void)setting;
    return ESP_OK;
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
        ESP_LOGI(TAG, "IDLE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_COMPLETED_PLAYING_NEXT:
        ESP_LOGI(TAG, "NEXT");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
        ESP_LOGI(TAG, "PLAYING");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
        ESP_LOGI(TAG, "PAUSE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_SHUTDOWN:
        ESP_LOGI(TAG, "SHUTDOWN");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN_FILE_TYPE:
        ESP_LOGI(TAG, "UNKNOWN FILE");
        break;
    case AUDIO_PLAYER_CALLBACK_EVENT_UNKNOWN:
        ESP_LOGI(TAG, "UNKNOWN");
        break;
    }
}

/* -------------------------------------------------------------------------- */
/*                     Clock reconfig + audio write hooks                     */
/*        These are called by chmorgan/esp-audio-player during playback       */
/* -------------------------------------------------------------------------- */

static esp_err_t audio_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    ESP_LOGI(TAG, "rate: %u", (unsigned int)rate);
    ESP_LOGI(TAG, "bits per sample: %u", (unsigned int)bits_cfg);
    ESP_LOGI(TAG, "channel: %d", ch);

    if (i2s_tx_chan == NULL) {
        ESP_LOGE(TAG, "I2S TX channel not initialized");
        return ESP_FAIL;
    }

    /* Build new clock config */
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate);

    /* Disable → Reconfig → Enable */
    ESP_ERROR_CHECK(i2s_channel_disable(i2s_tx_chan));
    ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(i2s_tx_chan, &clk_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));

    return ESP_OK;
}

static esp_err_t audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    if (i2s_tx_chan == NULL) {
        ESP_LOGE(TAG, "audio_write: I2S TX channel not initialized");
        return ESP_FAIL;
    }

    esp_err_t ret = i2s_channel_write(i2s_tx_chan, audio_buffer, len, bytes_written, timeout_ms);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "audio_write: i2s_channel_write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

/* -------------------------------------------------------------------------- */
/*                          Public initialisation API                         */
/* -------------------------------------------------------------------------- */

esp_err_t audio_play_start(void)
{
    esp_err_t ret = ESP_OK;

    /* Initialise I2S once */
    ESP_ERROR_CHECK(codec_init());

    audio_player_config_t config = {
        .mute_fn   = app_mute_function,
        .clk_set_fn = audio_reconfig_clk,
        .write_fn  = app_audio_write,
        .priority  = 5,
    };

    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_callback, NULL);

    return ret;
}
