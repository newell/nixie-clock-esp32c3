#include <time.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_random.h>
#include <driver/gpio.h>

#include "wifi_prov.h"
#include "vfs.h"
#include "sntp.h"
#include "clock.h"
#include "config.h"
#include "leds.h"
#include "ws_server.h"
#include "audio.h"
#include "motion.h"


#define HVEN GPIO_NUM_7

static const char *TAG = "main";

TaskHandle_t toggle_led_task_handle;
TaskHandle_t play_audio_task_handle;
TaskHandle_t slot_machine_effect_task_handle;

void toggle_led_task(void *pvParameters) {

    while (1) {
        // Wait for the signal to perform LED operations
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Party it up!
        party_lights();
    }
}

void play_audio_task(void *pvParameters) {

    while (1) {
        // Wait for the signal to perform audio playback
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        audio_handle_info(SOUND_TYPE_GOOD_FOOT);
    }
}

void slot_machine_effect_task(void *pvParameters) {

    while (1) {
        // Wait for the signal to perform slot machine effect
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Run slot machine effect and safely update the display
            slot_machine_effect();
            xSemaphoreGive(xMutex);
        }
    }
}

void hourly_task(void *pvParameters) {

    while (1) {
        // Get the current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Calculate how many seconds until the next hour
        int seconds_until_next_hour = (60 - timeinfo.tm_min) * 60 - timeinfo.tm_sec;

        ESP_LOGI(TAG, "Next hour in %d seconds", seconds_until_next_hour);

        // Wait until the next hour
        vTaskDelay(pdMS_TO_TICKS(seconds_until_next_hour * 1000));

        // Execute the block of code when the hour turns
        ESP_LOGI(TAG, "Hour changed! Executing task...");

        // // Calculate how many seconds until the next minute -- used for debugging
        // int seconds_until_next_minute = 60 - timeinfo.tm_sec;

        // ESP_LOGI(TAG, "Next minute in %d seconds", seconds_until_next_minute);

        // // Wait until the next minute
        // vTaskDelay(pdMS_TO_TICKS(seconds_until_next_minute * 1000));

        // // Execute the block of code when the hour turns
        // ESP_LOGI(TAG, "Minute changed! Executing task...");

        // Notify the child tasks to perform their actions
        xTaskNotifyGive(toggle_led_task_handle);
        xTaskNotifyGive(play_audio_task_handle);
        xTaskNotifyGive(slot_machine_effect_task_handle);
    }
}

void enable_hv(void) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;  // Disable interrupts for the GPIO
    io_conf.mode = GPIO_MODE_OUTPUT;        // Set GPIO mode as output
    io_conf.pin_bit_mask = (1ULL << HVEN);  // Replace XX with your GPIO number
    io_conf.pull_down_en = 0;               // Disable pull-down
    io_conf.pull_up_en = 0;                 // Disable pull-up
    gpio_config(&io_conf);                  // Configure the GPIO

    // Turn on the HV
    gpio_set_level(HVEN, 1);
}

void app_main(void) {

    // Clock Mutex
    xMutex = xSemaphoreCreateMutex();
    if (xMutex == NULL) {
        printf("Failed to create semaphore\n");
        return;
    }

    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize VFS */
    ESP_ERROR_CHECK(vfs_init());

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize Config */
    config_init();

    /* Initialize Wi-Fi */
    wifi_prov_init();

    /* Sync SNTP */
    sync_sntp();

    /* Configure the LEDs */
    configure_leds();

    /* Intialize Clock */
    clock_init();

    /* Enable High Voltage */
    enable_hv();

    /* Start the server for the first time */
    ESP_ERROR_CHECK(start_webserver());

    /* Initialize Sound */
    ESP_ERROR_CHECK(audio_play_start());

    /* Initialize Motion */
    // motion_init();

    // Create the hourly tasks
    xTaskCreate(toggle_led_task, "Toggle LED Task", 2048, NULL, 1, &toggle_led_task_handle);
    xTaskCreate(play_audio_task, "Play Audio Task", 2048, NULL, 1, &play_audio_task_handle);
    xTaskCreate(slot_machine_effect_task, "Slot Machine Effect Task", 2048, NULL, 1, &slot_machine_effect_task_handle);
    xTaskCreate(hourly_task, "Hourly Task", 2048, NULL, 1, NULL);
}
