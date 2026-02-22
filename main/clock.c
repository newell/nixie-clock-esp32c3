#include "clock.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "leds.h"

extern TaskHandle_t play_audio_task_handle;
static const char* TAG = "clock";
static int ram_time_fmt = 1;  // Default to 24h

// Display Queue System (Single Hardware Owner Model)
typedef enum { DISP_CMD_SHOW_TIME, DISP_CMD_SLOT_MACHINE } disp_cmd_type_t;

typedef struct {
    disp_cmd_type_t type;
} disp_msg_t;

static QueueHandle_t disp_queue = NULL;

// Shift Register Logic
uint32_t hours = 0;
uint32_t minutes = 0;
uint32_t seconds = 0;

void clock_set_ram_format(int fmt) {
    ram_time_fmt = fmt;
    ESP_LOGI(TAG, "Format updated: %s", (fmt == 0) ? "12h" : "24h");
}

void shift_out_data(uint64_t data) {
    for (int i = 63; i >= 0; i--) {
        gpio_set_level(DATA_PIN, (data >> i) & 0x01);
        gpio_set_level(CLOCK_PIN, 1);
        esp_rom_delay_us(2);
        gpio_set_level(CLOCK_PIN, 0);
        esp_rom_delay_us(2);
    }
}

uint64_t set_bit(uint64_t num, int pos) {
    return num | ((uint64_t)1 << pos);
}

uint64_t set_HH(uint64_t num, uint32_t HH) {
    static const int map[] = {7, 6, 5, 4, 3, 2, 1, 0, 15, 14};
    return (HH < 10) ? set_bit(num, 63 - map[HH]) : num;
}
uint64_t set_H(uint64_t num, uint32_t H) {
    static const int map[] = {13, 12, 11, 10, 9, 8, 23, 22, 21, 20};
    return (H < 10) ? set_bit(num, 63 - map[H]) : num;
}
uint64_t set_MM(uint64_t num, uint32_t MM) {
    static const int map[] = {17, 16, 31, 30, 29, 28, 27, 26, 25, 24};
    return (MM < 10) ? set_bit(num, 63 - map[MM]) : num;
}
uint64_t set_M(uint64_t num, uint32_t M) {
    static const int map[] = {39, 38, 37, 36, 35, 34, 33, 32, 47, 46};
    return (M < 10) ? set_bit(num, 63 - map[M]) : num;
}
uint64_t set_SS(uint64_t num, uint32_t SS) {
    static const int map[] = {43, 42, 41, 40, 55, 54, 53, 52, 51, 50};
    return (SS < 10) ? set_bit(num, 63 - map[SS]) : num;
}
uint64_t set_S(uint64_t num, uint32_t S) {
    static const int map[] = {49, 48, 63, 62, 61, 60, 59, 58, 57, 56};
    return (S < 10) ? set_bit(num, 63 - map[S]) : num;
}

void update_tubes(uint32_t HH, uint32_t H, uint32_t MM, uint32_t M, uint32_t SS,
                  uint32_t S, bool show_dots) {
    uint64_t num = 0;

    num = set_HH(num, HH);
    num = set_H(num, H);
    num = set_MM(num, MM);
    num = set_M(num, M);
    num = set_SS(num, SS);
    num = set_S(num, S);

    if (show_dots) {
        num = set_bit(num, 18);
        num = set_bit(num, 19);
        num = set_bit(num, 44);
        num = set_bit(num, 45);
    }

    shift_out_data(num);
    gpio_set_level(LATCH_PIN, 1);
    esp_rom_delay_us(5);
    gpio_set_level(LATCH_PIN, 0);
}

static void update_shift_registers(void) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    seconds = timeinfo.tm_sec;

    if (ram_time_fmt == 0) {
        hours %= 12;
        if (hours == 0) hours = 12;
    }

    bool dots = (seconds % 2 == 0);

    update_tubes((hours / 10), (hours % 10), (minutes / 10), (minutes % 10),
                 (seconds / 10), (seconds % 10), dots);
}

static void slot_machine_effect(void) {
    for (int i = 0; i < 120; i++) {
        update_tubes(esp_random() % 10, esp_random() % 10, esp_random() % 10,
                     esp_random() % 10, esp_random() % 10, esp_random() % 10,
                     false);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void update_clock_task(void* pvParameters) {
    ESP_LOGI(TAG, "Clock task started");

    disp_msg_t msg;

    while (1) {
        // Wait up to 1 second for command
        if (xQueueReceive(disp_queue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
            switch (msg.type) {
                case DISP_CMD_SLOT_MACHINE:
                    slot_machine_effect();
                    break;

                case DISP_CMD_SHOW_TIME:
                default:
                    update_shift_registers();
                    break;
            }

        } else {
            // Timeout every second → normal time update
            update_shift_registers();
        }
    }
}

// Wrapper: Slot Machine + LEDs + Audio
void clock_send_slot_machine_with_leds(void) {
    ESP_LOGI(TAG, "Starting slot machine effect with LEDs and audio");

    led_send_msg(LED_CMD_SLOT_MODE, 0, 0, 0);
    if (play_audio_task_handle) {
        xTaskNotifyGive(play_audio_task_handle);
    } else {
        ESP_LOGW(TAG, "Audio task handle not initialized");
    }

    // Run the slot machine tubes animation
    slot_machine_effect();

    ESP_LOGI(TAG, "Slot machine effect with LEDs and audio complete");
}

void clock_init(void) {
    gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                             .mode = GPIO_MODE_OUTPUT,
                             .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,
                             .pull_down_en = 0,
                             .pull_up_en = 0};

    gpio_config(&io_conf);

    gpio_set_level(OE_PIN, 0);
    gpio_set_level(CLOCK_PIN, 0);
    gpio_set_level(LATCH_PIN, 0);

    // Load time format from config
    char time_fmt_value[2] = {0};
    read_config_value("time_fmt", time_fmt_value, sizeof(time_fmt_value));
    if (time_fmt_value[0] != '\0') {
        ram_time_fmt = atoi(time_fmt_value);
    }

    // Create display queue
    disp_queue = xQueueCreate(5, sizeof(disp_msg_t));

    // Blank display at startup
    update_tubes(10, 10, 10, 10, 10, 10, false);

    // Create display task (ONLY hardware owner)
    xTaskCreate(update_clock_task, "clk_task", 4096, NULL, 5, NULL);
}
