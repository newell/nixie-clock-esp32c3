#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_netif.h>

#include "wifi_prov.h"
#include "vfs.h"
#include "config.h"
#include "sntp.h"
#include "clock.h"
#include "leds.h"
#include "ws_server.h"
#include "audio.h"
#include "motion.h"


void app_main(void)
{
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize Wi-Fi */
    wifi_prov_init();

    /* Initialize VFF */
    ESP_ERROR_CHECK(vfs_init());

    /* Config file */
    config_init();

    /* Sync SNTP */
    sync_sntp();

    /* Intialize Clock */
    clock_init();

    /* Configure the LEDs */
    configure_leds();

    /* Start the server for the first time */
    ESP_ERROR_CHECK(start_webserver());

    /* Initialize Sound */
    ESP_ERROR_CHECK(audio_play_start());

    audio_handle_info(SOUND_TYPE_GOOD_FOOT);

    // for (int i=0; i < 10; i++) {
    //     audio_handle_info(SOUND_TYPE_GOOD_FOOT);
    //     vTaskDelay(pdMS_TO_TICKS(10000));
    // }

    /* Initialize Sound */
    motion_init();

}
