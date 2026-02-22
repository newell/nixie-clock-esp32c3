#pragma once

#include <stdint.h>

#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

// Type of led strip encoder configuration
typedef struct {
    uint32_t resolution;  // Encoder resolution, in Hz
} led_strip_encoder_config_t;

// Create RMT encoder for encoding LED strip pixels into RMT symbols
esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t* config,
                                    rmt_encoder_handle_t* ret_encoder);

#ifdef __cplusplus
}
#endif
