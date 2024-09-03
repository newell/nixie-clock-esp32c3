#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/rmt_tx.h>
#include <led_strip_encoder.h>
#include <driver/gpio.h>

#include "config.h"
#include "leds.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      8

#define EXAMPLE_LED_NUMBERS         6
#define EXAMPLE_CHASE_SPEED_MS      100
#define EXAMPLE_CHASE_SPEEDUP_MS      10

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];

static const char *TAG = "LED";

led_strip_handle_t led_strip;

/* Simple helper function, converting HSV color space to RGB color space */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
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

void set_leds_to_config(led_strip_handle_t *led_strip) {

    /* Set LEDs to configuration color */
    char r_value[4];
    char g_value[4];
    char b_value[4];

    read_config_value("r", r_value, sizeof(r_value));
    read_config_value("g", g_value, sizeof(g_value));
    read_config_value("b", b_value, sizeof(b_value));

    // Convert the obtained strings to integers
    int red = atoi(r_value);
    int green = atoi(g_value);
    int blue = atoi(b_value);

    for (int i=0; i < 6; i++) {
        led_strip_set_pixel(*led_strip, i, red, green, blue);
        led_strip_refresh(*led_strip); // refresh to send the data
    }
}

void party_lights(void)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    ESP_LOGI(TAG, "Start LED rainbow chase");

    for (int k = 0; k < 10; k++) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);

                // Set pixel color using led_strip_api
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, red, green, blue));
            }
            // Apply the changes to the strip
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));

            // Clear the strip by setting all pixels to black (0,0,0)
            for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, 0, 0, 0));
            }
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        start_rgb += 60;
    }

    for (int k = 0; k < 100; k++) {
        for (int i = 0; i < 3; i++) {
            for (int j = i; j < EXAMPLE_LED_NUMBERS; j += 3) {
                // Build RGB pixels
                hue = j * 360 / EXAMPLE_LED_NUMBERS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);

                // Set pixel color using led_strip_api
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, red, green, blue));
            }
            // Apply the changes to the strip
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEEDUP_MS));

            // Clear the strip by setting all pixels to black (0,0,0)
            for (int j = 0; j < EXAMPLE_LED_NUMBERS; j++) {
                ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, j, 0, 0, 0));
            }
            ESP_ERROR_CHECK(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEEDUP_MS));
        }
        start_rgb += 60;
    }

    // After the sequence, reset LEDs to the configuration color
    set_leds_to_config(&led_strip);
}

void configure_leds(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 6, // six LEDs on board
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812, // LED strip model
        .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    set_leds_to_config(&led_strip);
}
