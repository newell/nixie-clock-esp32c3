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
#include <driver/i2s_pdm.h>
#include <audio_player.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>

#include "audio.h"

static const char *TAG = "audio";

static const audio_codec_data_if_t *i2s_data_if = NULL;  /* Codec data interface */
static i2s_chan_handle_t i2s_tx_chan;

static esp_codec_dev_handle_t play_dev_handle;

static esp_err_t audio_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);
static esp_err_t audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);


static esp_err_t audio_init(const i2s_std_config_t *i2s_config, i2s_chan_handle_t *tx_channel)
{
        if (i2s_tx_chan && i2s_data_if) {
        if (tx_channel) {
            *tx_channel = i2s_tx_chan;
        }

        /* Audio was initialized before */
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_channel, NULL));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = I2S_DUPLEX_STEREO_CFG(44100);
    const i2s_std_config_t *p_i2s_cfg = &std_cfg_default;
    if (i2s_config != NULL) {
        p_i2s_cfg = i2s_config;
    }

    if (tx_channel != NULL) {
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(*tx_channel, p_i2s_cfg));
        ESP_ERROR_CHECK(i2s_channel_enable(*tx_channel));
    }

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = NULL,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);
    NULL_CHECK(i2s_data_if, NULL);

    return ESP_OK;
}

esp_codec_dev_handle_t audio_codec_speaker_init(void)
{

    if (i2s_tx_chan == NULL || i2s_data_if == NULL) {
        /* Configure I2S peripheral and Power Amplifier */
        ERROR_CHECK_RETURN_ERR(audio_init(NULL, &i2s_tx_chan));
    }
    assert(i2s_data_if);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = NULL,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

esp_err_t app_audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    if (audio_write(audio_buffer, len, bytes_written, 1000) != ESP_OK) {
        ESP_LOGE(TAG, "Write Task: i2s write failed");
        ret = ESP_FAIL;
    }

    return ret;
}

esp_err_t audio_handle_info(PDM_SOUND_TYPE voice)
{
    char filepath[30];
    esp_err_t ret = ESP_OK;

    switch (voice) {
    case SOUND_TYPE_HIT_ME:
        sprintf(filepath, "%s/%s", CONFIG_SPIFFS_MOUNT_POINT, "HitMe.mp3");
        break;
    case SOUND_TYPE_GOOD_FOOT:
        sprintf(filepath, "%s/%s", CONFIG_SPIFFS_MOUNT_POINT, "GetOnGoodFoot.mp3");
        break;
    }

    FILE *fp = fopen(filepath, "r");
    ESP_GOTO_ON_FALSE(fp, ESP_FAIL, err, TAG, "Failed to open file: %s", filepath);

    ESP_LOGI(TAG, "play: %s", filepath);
    ret = audio_player_play(fp);
err:
    return ret;
}

static esp_err_t app_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    return ESP_OK;
}

static void audio_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case AUDIO_PLAYER_CALLBACK_EVENT_IDLE: /**< Player is idle, not playing audio */
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

static esp_err_t audio_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "rate: %u", (unsigned int)rate);
    ESP_LOGI(TAG, "bits per sample: %u", (unsigned int)bits_cfg);
    ESP_LOGI(TAG, "channel: %d", ch);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_close(play_dev_handle);
    ret = esp_codec_dev_open(play_dev_handle, &fs);
    return ret;
}

static esp_err_t audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

static void codec_init()
{
    play_dev_handle = audio_codec_speaker_init();
    assert((play_dev_handle) && "play_dev_handle not initialized");
}

esp_err_t audio_play_start()
{
    esp_err_t ret = ESP_OK;

    codec_init();

    audio_player_config_t config = {
        .mute_fn = app_mute_function,
        .write_fn = app_audio_write,
        .clk_set_fn = audio_reconfig_clk,
        .priority = 5
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_callback, NULL);
    return ret;
}
