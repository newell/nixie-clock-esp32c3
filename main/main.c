#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "clock.h"
#include "config.h"
#include "leds.h"
#include "sntp.h"
#include "vfs.h"
#include "wifi_prov.h"
#include "ws_server.h"

#define HVEN GPIO_NUM_7
static const char* TAG = "main";

TaskHandle_t led_slot_machine_task_handle = NULL;
TaskHandle_t play_audio_task_handle = NULL;
TaskHandle_t led_mode_task_handle = NULL;

void led_slot_machine_task(void* pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "LED Slot Triggered");
        led_send_msg(LED_CMD_SLOT_MODE, 0, 0, 0);
    }
}

void play_audio_task(void* pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "Playing Hourly Audio");
        audio_handle_info(SOUND_TYPE_GOOD_FOOT);
    }
}

void hourly_task(void* pvParameters) {
    while (1) {
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        int seconds_until_next_hour =
            (60 - timeinfo.tm_min) * 60 - timeinfo.tm_sec;
        if (seconds_until_next_hour <= 0) seconds_until_next_hour = 3600;
        vTaskDelay(pdMS_TO_TICKS(seconds_until_next_hour * 1000));
        ESP_LOGI(TAG, "Hour changed!");

        // /* DEBUG: Trigger every minute */
        // int seconds_until_next_minute = 60 - timeinfo.tm_sec;
        // if (seconds_until_next_minute <= 0) seconds_until_next_minute = 60;

        // vTaskDelay(pdMS_TO_TICKS(seconds_until_next_minute * 1000));

        clock_send_slot_machine_with_leds();
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(vfs_init());
    ESP_ERROR_CHECK(esp_netif_init());

    config_init();

    // Sync LED mode from config to RAM
    char saved_mode[16] = {0};
    read_config_value("led_mode", saved_mode, sizeof(saved_mode));

    if (strlen(saved_mode) > 0) {
        led_set_ram_mode(saved_mode);
    } else {
        led_set_ram_mode("static");
    }

    wifi_prov_init();
    sync_sntp();

    configure_leds();
    clock_init();  // IMPORTANT: Initialize clock BEFORE hourly task

    // High Voltage Enable
    gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                             .mode = GPIO_MODE_OUTPUT,
                             .pin_bit_mask = (1ULL << HVEN),
                             .pull_down_en = 0,
                             .pull_up_en = 0};
    gpio_config(&io_conf);
    gpio_set_level(HVEN, 1);

    ESP_ERROR_CHECK(start_webserver());
    ESP_ERROR_CHECK(audio_play_start());

    // Create Tasks
    xTaskCreate(led_task, "LED Master", 4096, NULL, 5, NULL);
    xTaskCreate(led_mode_task, "LED Mode", 2048, NULL, 2,
                &led_mode_task_handle);
    xTaskCreate(led_slot_machine_task, "Slot Trigger", 2048, NULL, 3,
                &led_slot_machine_task_handle);
    xTaskCreate(play_audio_task, "Play Audio", 4096, NULL, 4,
                &play_audio_task_handle);
    xTaskCreate(hourly_task, "Hourly", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "System initialization complete");
}
