#ifndef LEDS_H
#define LEDS_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <led_strip.h>

// Commands for the LED consumer task
typedef enum {
    LED_CMD_SET_COLOR,
    LED_CMD_SLOT_MODE,
    LED_CMD_SPECTRUM_STEP,
    LED_CMD_RELOAD_CONFIG
} led_msg_type_t;

typedef struct {
    led_msg_type_t type;
    struct {
        uint8_t r, g, b;
    } color;
} led_msg_t;

// Global queue handle
extern QueueHandle_t led_queue;

void configure_leds(void);
void led_task(void* pvParameters);
void led_mode_task(void* pvParameters);
void led_send_msg(led_msg_type_t type, uint8_t r, uint8_t g, uint8_t b);
void led_set_ram_color(uint8_t r, uint8_t g, uint8_t b);
void led_set_ram_mode(const char* mode);

#endif /* LEDS_H */
