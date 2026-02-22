#include "leds.h"

#include <driver/rmt_tx.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip_encoder.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define RMT_LED_STRIP_GPIO_NUM 8
#define EXAMPLE_LED_NUMBERS 6
#define EXAMPLE_CHASE_SPEED_MS 100
#define EXAMPLE_CHASE_SPEEDUP_MS 10

static const char* TAG = "LED_CORE";
led_strip_handle_t led_strip;
QueueHandle_t led_queue = NULL;

static char current_led_mode[16] = "static";
static uint8_t ram_r = 0, ram_g = 0, ram_b = 0;

void led_set_ram_color(uint8_t r, uint8_t g, uint8_t b) {
    ram_r = r;
    ram_g = g;
    ram_b = b;

    // If we are in static mode, apply this color immediately to RAM
    if (strcmp(current_led_mode, "static") == 0) {
        led_send_msg(LED_CMD_SET_COLOR, ram_r, ram_g, ram_b);
    }
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t* r,
                       uint32_t* g, uint32_t* b) {
    h %= 360;
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;
    uint32_t i = h / 60;
    uint32_t diff = h % 60;
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
        case 0:
            *r = rgb_max;
            *g = rgb_min + rgb_adj;
            *b = rgb_min;
            break;
        case 1:
            *r = rgb_max - rgb_adj;
            *g = rgb_max;
            *b = rgb_min;
            break;
        case 2:
            *r = rgb_min;
            *g = rgb_max;
            *b = rgb_min + rgb_adj;
            break;
        case 3:
            *r = rgb_min;
            *g = rgb_max - rgb_adj;
            *b = rgb_max;
            break;
        case 4:
            *r = rgb_min + rgb_adj;
            *g = rgb_min;
            *b = rgb_max;
            break;
        default:
            *r = rgb_max;
            *g = rgb_min;
            *b = rgb_max - rgb_adj;
            break;
    }
}

static void apply_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
        led_strip_set_pixel(led_strip, i, r, g, b);
    }
    led_strip_refresh(led_strip);
}

static void load_nvs_to_ram(void) {
    char mode_val[16] = {0};
    char r_val[4] = {0}, g_val[4] = {0}, b_val[4] = {0};

    // Get Mode
    read_config_value("led_mode", mode_val, sizeof(mode_val));
    if (mode_val[0] != '\0') {
        led_set_ram_mode(mode_val);
    }

    // Load Colors
    read_config_value("r", r_val, sizeof(r_val));
    read_config_value("g", g_val, sizeof(g_val));
    read_config_value("b", b_val, sizeof(b_val));

    // If the buffer isn't empty, convert and store in RAM
    if (r_val[0] != '\0') ram_r = atoi(r_val);
    if (g_val[0] != '\0') ram_g = atoi(g_val);
    if (b_val[0] != '\0') ram_b = atoi(b_val);
}

void led_send_msg(led_msg_type_t type, uint8_t r, uint8_t g, uint8_t b) {
    if (!led_queue) return;
    led_msg_t msg = {.type = type, .color = {r, g, b}};
    xQueueSend(led_queue, &msg, 0);
}

static void internal_slot_machine_lights(void) {
    uint32_t r, g, b;
    uint16_t hue = 0, start_rgb = 0;
    for (int loop = 0; loop < 2; loop++) {
        int delay =
            (loop == 0) ? EXAMPLE_CHASE_SPEED_MS : EXAMPLE_CHASE_SPEEDUP_MS;
        for (int k = 0; k < 10; k++) {
            for (int i = 0; i < 3; i++) {
                for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                    hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                    led_strip_hsv2rgb(hue, 100, 100, &r, &g, &b);
                    led_strip_set_pixel(led_strip, j, r, g, b);
                }
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(delay));
                for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++)
                    led_strip_set_pixel(led_strip, j, 0, 0, 0);
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(delay));
            }
            start_rgb += 60;
        }
    }
    // After slot-machine, return to the RAM colors
    apply_color(ram_r, ram_g, ram_b);
}

void led_task(void* pvParameters) {
    led_msg_t msg;
    led_queue = xQueueCreate(10, sizeof(led_msg_t));

    // Initial boot load
    load_nvs_to_ram();
    apply_color(ram_r, ram_g, ram_b);

    while (1) {
        if (xQueueReceive(led_queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch (msg.type) {
                case LED_CMD_SET_COLOR:
                    apply_color(msg.color.r, msg.color.g, msg.color.b);
                    break;
                case LED_CMD_SLOT_MODE:
                    internal_slot_machine_lights();
                    break;
                case LED_CMD_RELOAD_CONFIG:
                    apply_color(ram_r, ram_g, ram_b);
                    break;
                case LED_CMD_SPECTRUM_STEP: {
                    // We use a larger range (0 to 3600) to act as "sub-degrees"
                    static uint32_t h_high_res = 0;
                    uint32_t r, g, b;

                    // We divide by 10 to get the 0-360 value the function
                    // expects. This effectively holds each color for 10 steps
                    // before moving 1 degree.
                    led_strip_hsv2rgb(h_high_res / 10, 100, 100, &r, &g, &b);
                    apply_color(r, g, b);

                    // Increment by 1 to move 0.1 degrees per step.
                    // To go even slower, increment by 1 and change the 3600/10
                    // to 36000/100.
                    h_high_res = (h_high_res + 7) % 3600;
                    break;
                }
            }
        }
    }
}

void led_mode_task(void* pvParameters) {
    bool was_spectrum = false;
    while (1) {
        if (strcmp(current_led_mode, "spectrum") == 0) {
            led_send_msg(LED_CMD_SPECTRUM_STEP, 0, 0, 0);
            was_spectrum = true;
            vTaskDelay(pdMS_TO_TICKS(40));
        } else {
            if (was_spectrum) {
                led_send_msg(LED_CMD_RELOAD_CONFIG, 0, 0, 0);
                was_spectrum = false;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void led_set_ram_mode(const char* mode) {
    if (mode == NULL) return;
    strncpy(current_led_mode, mode, sizeof(current_led_mode) - 1);
    current_led_mode[sizeof(current_led_mode) - 1] = '\0';
    ESP_LOGI(TAG, "Mode set to: %s", current_led_mode);
}

void configure_leds(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .max_leds = EXAMPLE_LED_NUMBERS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
    };
    led_strip_rmt_config_t rmt_config = {.resolution_hz = 10 * 1000 * 1000};
    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}
