#ifndef AUDIO_H
#define AUDIO_H

#define ERROR_CHECK_RETURN_ERR(x)    ESP_ERROR_CHECK(x)
#define ERROR_CHECK_RETURN_NULL(x)   ESP_ERROR_CHECK(x)
#define ERROR_CHECK(x, ret)          ESP_ERROR_CHECK(x)
#define NULL_CHECK(x, ret)           assert(x)
#define NULL_CHECK_GOTO(x, goto_tag) assert(x)

/* Audio */
#define I2S_BCLK    GPIO_NUM_0
#define I2S_LRCLK   GPIO_NUM_1
#define I2S_DOUT    GPIO_NUM_2

/**
 * @brief I2S pinout
 *
 * Can be used for i2s_std_gpio_config_t and/or i2s_std_config_t initialization
 */
#define I2S_GPIO_CFG            \
    {                          \
        .bclk = I2S_BCLK,  \
        .ws = I2S_LRCLK,   \
        .dout = I2S_DOUT,  \
        .invert_flags = {      \
            .bclk_inv = false,  \
            .ws_inv = false,    \
        },                     \
    }

/**
 * @brief Mono Duplex I2S configuration structure
 *
 * This configuration is used by default in audio_init()
 */
#define I2S_DUPLEX_STEREO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), \
        .gpio_cfg = I2S_GPIO_CFG,                                                                 \
    }

typedef enum {
    SOUND_TYPE_HIT_ME,
    SOUND_TYPE_GOOD_FOOT,
} PDM_SOUND_TYPE;

esp_err_t audio_handle_info(PDM_SOUND_TYPE voice);

esp_err_t audio_play_start();

#endif /* AUDIO_H */


