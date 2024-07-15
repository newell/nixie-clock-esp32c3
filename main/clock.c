#include <time.h>
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "audio.h"
#include "config.h"
#include "clock.h"


static const char *TAG = "clock";

// Global variables for clock time
uint32_t hours = 0;
uint32_t minutes = 0;
uint32_t seconds = 0;

// int counter = 0;

// THESE DIGITS ARE FLIPPED DUE TO INCORRECT WIRING
// See below commentd out for correct ordering of digits
uint8_t digits[10][4] = {
    {0,0,0,0}, // 0
    {1,0,0,1}, // 9
    {1,0,0,0}, // 8
    {0,1,1,1}, // 7
    {0,1,1,0}, // 6
    {0,1,0,1}, // 5
    {0,1,0,0}, // 4
    {0,0,1,1}, // 3
    {0,0,1,0}, // 2
    {0,0,0,1}}; // 1


/* Future */
// BCD uses {D, C, B, A}
// Thus, if we are writing MSB that means we have the SR have:
// ABCDABCD, where the LSB of this is tied to the bottom of the SR
// which is correct, the issue was I had the Nixie tubes wired incorrectly.
// For example, 1 was tied to 9 position on the BCD driver, etc.
// uint8_t digits[10][4] = {
//     {0,0,0,0}, // 0
//     {0,0,0,1}, // 1
//     {0,0,1,0}, // 2
//     {0,0,1,1}, // 3
//     {0,1,0,0}, // 4
//     {0,1,0,1}, // 5
//     {0,1,1,0}, // 6
//     {0,1,1,1}, // 7
//     {1,0,0,0}, // 8
//     {1,0,0,1}}; // 9

void toggleOneTube (int i) {
    gpio_set_level(DATA_PIN, digits[i][0]);
    gpio_set_level(CLOCK_PIN, 1);
    gpio_set_level(CLOCK_PIN, 0);
    gpio_set_level(DATA_PIN, digits[i][1]);
    gpio_set_level(CLOCK_PIN, 1);
    gpio_set_level(CLOCK_PIN, 0);
    gpio_set_level(DATA_PIN, digits[i][2]);
    gpio_set_level(CLOCK_PIN, 1);
    gpio_set_level(CLOCK_PIN, 0);
    gpio_set_level(DATA_PIN, digits[i][3]);
    gpio_set_level(CLOCK_PIN, 1);
    gpio_set_level(CLOCK_PIN, 0);
}

void toggleSixTubes (int i) {
    for (int j = 0; j < 6; j++) {
        toggleOneTube(i);
    }
}

// Task function to update clock time
static void update_clock_task(void *pvParameters) {
    while (1) {
        // Get current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Update clock variables
        hours = timeinfo.tm_hour;
        minutes = timeinfo.tm_min;
        seconds = timeinfo.tm_sec;

        // Get time_fmt
        char time_fmt_value[2];
        read_config_value("time_fmt", time_fmt_value, sizeof(time_fmt_value));

        if (strcmp(time_fmt_value, "0") == 0) {
            hours = hours % 12;
            if (hours == 0) {
                hours = 12;
            }
        }

        // Print updated time
        ESP_LOGI(TAG, "Time: %02u:%02u:%02u", (unsigned int) hours, (unsigned int) minutes, (unsigned int) seconds);

        // TODO - Retrieve the time_fmt string from the config file and if it is equal to "0" I will
        // need to change the hours to modulo-12


        // TODO - uncomment the future section below, we are using this one becaue
        // the breadborad is wired incorrectly.
        toggleOneTube(seconds % 10);
        toggleOneTube(seconds / 10);
        toggleOneTube(minutes % 10);
        toggleOneTube(minutes / 10);
        toggleOneTube(hours % 10);
        toggleOneTube(hours / 10);
        gpio_set_level(LATCH_PIN, 1);
        gpio_set_level(LATCH_PIN, 0);

        /* Future */
        // toggleOneTube(seconds / 10);
        // toggleOneTube(seconds % 10);
        // toggleOneTube(minutes / 10);
        // toggleOneTube(minutes % 10);
        // toggleOneTube(hours / 10);
        // toggleOneTube(hours % 10);
        // gpio_set_level(LATCH_PIN, 1);
        // gpio_set_level(LATCH_PIN, 0);

        /* Counter test */
        // toggleSixTubes(counter);
        // gpio_set_level(LATCH_PIN, 1);
        // gpio_set_level(LATCH_PIN, 0);
        // counter++;
        // if (counter == 10) {
        //     counter = 0;
        // }

        // Delay for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void clock_init(void) {

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(LATCH_PIN, 0);

    // Create task to update clock time
    xTaskCreate(update_clock_task, "update_clock_task", 4096, NULL, 5, NULL);
}
