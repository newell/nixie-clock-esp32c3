#include <time.h>
#include <stdio.h>
#include <stdint.h>
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
bool indicator = false;
uint32_t hours = 0;
uint32_t minutes = 0;
uint32_t seconds = 0;

// Declare a 64-bit unsigned integer and initialize it to 0
uint64_t num = 0;

// Function to print binary representation of a 64-bit integer
void print_binary(uint64_t num) {
    // Iterate from the MSB to the LSB
    for (int i = 63; i >= 0; i--) {
        // Extract the bit at position i
        int bit = (num >> i) & 1;
        // Print the bit
        printf("%d", bit);
        // Print a space every 8 bits for readability
        if (i % 8 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}

void shift_out_data(uint64_t data) {
    for (int i = 63; i >= 0; i--) {  // LSB first
        gpio_set_level(DATA_PIN, (data >> i) & 0x01);  // Set DATA_PIN to current bit
        gpio_set_level(CLOCK_PIN, 1);
        esp_rom_delay_us(10);  // Small delay to ensure the pulse is registered
        gpio_set_level(CLOCK_PIN, 0);
    }
}

// Function to set a bit
uint64_t set_bit(uint64_t num, int pos) {
    return num | ((uint64_t)1 << pos);
}

// Function to clear a bit
uint64_t clear_bit(uint64_t num, int pos) {
    return num & ~((uint64_t)1 << pos);
}

// Function to toggle a bit
uint64_t toggle_bit(uint64_t num, int pos) {
    return num ^ ((uint64_t)1 << pos);
}

uint64_t set_HH(uint64_t num, uint32_t HH) {
    if (HH == 7) {
        return set_bit(num, 63 - 0);
    } else if (HH == 6) {
        return set_bit(num, 63 - 1);
    } else if (HH == 5) {
        return set_bit(num, 63 - 2);
    } else if (HH == 4) {
        return set_bit(num, 63 - 3);
    } else if (HH == 3) {
        return set_bit(num, 63 - 4);
    } else if (HH == 2) {
        return set_bit(num, 63 - 5);
    } else if (HH == 1) {
        return set_bit(num, 63 - 6);
    } else if (HH == 0) {
        return set_bit(num, 63 - 7);
    } else if (HH == 9) {
        return set_bit(num, 63 - 14);
    } else if (HH == 8) {
        return set_bit(num, 63 - 15);
    } else {
        return num;
    }
}

uint64_t set_H(uint64_t num, uint32_t H) {
    if (H == 5) {
        return set_bit(num, 63 - 8);
    } else if (H == 4) {
        return set_bit(num, 63 - 9);
    } else if (H == 3) {
        return set_bit(num, 63 - 10);
    } else if (H == 2) {
        return set_bit(num, 63 - 11);
    } else if (H == 1) {
        return set_bit(num, 63 - 12);
    } else if (H == 0) {
        return set_bit(num, 63 - 13);
    } else if (H == 9) {
        return set_bit(num, 63 - 20);
    } else if (H == 8) {
        return set_bit(num, 63 - 21);
    } else if (H == 7) {
        return set_bit(num, 63 - 22);
    } else if (H == 6) {
        return set_bit(num, 63 - 23);
    } else {
        return num;
    }
}


uint64_t set_MM(uint64_t num, uint32_t MM) {
    if (MM == 1) {
        return set_bit(num, 63 - 16);
    } else if (MM == 0) {
        return set_bit(num, 63 - 17);
    } else if (MM == 9) {
        return set_bit(num, 63 - 24);
    } else if (MM == 8) {
        return set_bit(num, 63 - 25);
    } else if (MM == 7) {
        return set_bit(num, 63 - 26);
    } else if (MM == 6) {
        return set_bit(num, 63 - 27);
    } else if (MM == 5) {
        return set_bit(num, 63 - 28);
    } else if (MM == 4) {
        return set_bit(num, 63 - 29);
    } else if (MM == 3) {
        return set_bit(num, 63 - 30);
    } else if (MM == 2) {
        return set_bit(num, 63 - 31);
    } else {
        return num;
    }
}

uint64_t set_M(uint64_t num, uint32_t M) {
    if (M == 7) {
        return set_bit(num, 63 - 32);
    } else if (M == 6) {
        return set_bit(num, 63 - 33);
    } else if (M == 5) {
        return set_bit(num, 63 - 34);
    } else if (M == 4) {
        return set_bit(num, 63 - 35);
    } else if (M == 3) {
        return set_bit(num, 63 - 36);
    } else if (M == 2) {
        return set_bit(num, 63 - 37);
    } else if (M == 1) {
        return set_bit(num, 63 - 38);
    } else if (M == 0) {
        return set_bit(num, 63 - 39);
    } else if (M == 9) {
        return set_bit(num, 63 - 46);
    } else if (M == 8) {
        return set_bit(num, 63 - 47);
    } else {
        return num;
    }
}

uint64_t set_SS(uint64_t num, uint32_t SS) {
    if (SS == 3) {
        return set_bit(num, 63 - 40);
    } else if (SS == 2) {
        return set_bit(num, 63 - 41);
    } else if (SS == 1) {
        return set_bit(num, 63 - 42);
    } else if (SS == 0) {
        return set_bit(num, 63 - 43);
    } else if (SS == 9) {
        return set_bit(num, 63 - 50);
    } else if (SS == 8) {
        return set_bit(num, 63 - 51);
    } else if (SS == 7) {
        return set_bit(num, 63 - 52);
    } else if (SS == 6) {
        return set_bit(num, 63 - 53);
    } else if (SS == 5) {
        return set_bit(num, 63 - 54);
    } else if (SS == 4) {
        return set_bit(num, 63 - 55);
    } else {
        return num;
    }
}

uint64_t set_S(uint64_t num, uint32_t S) {
    if (S == 1) {
        return set_bit(num, 63 - 48);
    } else if (S == 0) {
        return set_bit(num, 63 - 49);
    } else if (S == 9) {
        return set_bit(num, 63 - 56);
    } else if (S == 8) {
        return set_bit(num, 63 - 57);
    } else if (S == 7) {
        return set_bit(num, 63 - 58);
    } else if (S == 6) {
        return set_bit(num, 63 - 59);
    } else if (S == 5) {
        return set_bit(num, 63 - 60);
    } else if (S == 4) {
        return set_bit(num, 63 - 61);
    } else if (S == 3) {
        return set_bit(num, 63 - 62);
    } else if (S == 2) {
        return set_bit(num, 63 - 63);
    } else {
        return num;
    }
}

// Function to update shift registers based on current time
void update_shift_registers(void) {
    time_t now;
    struct tm timeinfo;

    // Get current time
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

    // Reset all the bits to zero
    num = 0;
    // Set time
    num = set_HH(num, (hours / 10));
    num = set_H(num, (hours % 10));
    num = set_MM(num, (minutes / 10));
    num = set_M(num, (minutes % 10));
    num = set_SS(num, (seconds / 10));
    num = set_S(num, (seconds % 10));
    // Set indicators
    if (indicator) {
        num = toggle_bit(num, 18);
        num = toggle_bit(num, 19);
        num = toggle_bit(num, 44);
        num = toggle_bit(num, 45);
        indicator = false;
    } else {
        num = clear_bit(num, 18);
        num = clear_bit(num, 19);
        num = clear_bit(num, 44);
        num = clear_bit(num, 45);
        indicator = true;
    }

    // Print binary
    print_binary(num);
    // Shift out all the data
    shift_out_data(num);

    // Latch data to outputs
    gpio_set_level(LATCH_PIN, 1);
    esp_rom_delay_us(10);  // Small delay to ensure the pulse is registered
    gpio_set_level(LATCH_PIN, 0);
}

// Task function to update clock time
static void update_clock_task(void *pvParameters) {
    while (1) {
        // Update shift registers based on current time
        update_shift_registers();

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

    gpio_set_level(OE_PIN, 0);
    gpio_set_level(CLOCK_PIN, 0);
    gpio_set_level(LATCH_PIN, 0);

    // Shift out all the data
    shift_out_data(num);

    // Latch data to outputs
    gpio_set_level(LATCH_PIN, 1);
    esp_rom_delay_us(10);  // Small delay to ensure the pulse is registered
    gpio_set_level(LATCH_PIN, 0);


    // Create task to update clock time
    xTaskCreate(update_clock_task, "update_clock_task", 4096, NULL, 5, NULL);
}

