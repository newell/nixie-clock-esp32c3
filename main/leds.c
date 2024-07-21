#include <driver/gpio.h>
#include <led_strip.h>

#include "config.h"
#include "leds.h"

led_strip_handle_t led_strip;

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
        led_strip_set_pixel(led_strip, i, red, green, blue);
        led_strip_refresh(led_strip); // refresh to send the data
    }
}
