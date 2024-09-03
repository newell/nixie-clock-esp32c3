#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <protocol_examples_common.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>
#include <cJSON.h>

#include "config.h"
#include "sntp.h"


static const char *TAG = "sntp";

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_time(void)
{
    ESP_LOGI(TAG, "Initializing and starting SNTP");

    // SNTP configuration
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
    config.sync_cb = time_sync_notification_cb;

    esp_netif_sntp_init(&config);

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    // localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "(obtain_time) -The current date/time is: %s", strftime_buf);

}

void sync_sntp(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        time(&now);
    }

    char timezone_str[32] = "";
    FILE *file = fopen(CONFIG_FILENAME, "r");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size <= 0) {
            ESP_LOGE(TAG, "File is empty or error determining size");
            fclose(file);
            return;
        }

        char *buffer = (char *)malloc(file_size + 1);
        if (buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for file buffer");
            fclose(file);
            return;
        }

        fread(buffer, 1, file_size, file);
        fclose(file);
        buffer[file_size] = '\0';

        ESP_LOGI(TAG, "buffer: %s", buffer);

        cJSON *root = cJSON_Parse(buffer);
        if (root == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            free(buffer);
            return;
        }

        cJSON *time_json = cJSON_GetObjectItem(root, "time");
        if (time_json != NULL && cJSON_IsObject(time_json)) {
            cJSON *timezone_json = cJSON_GetObjectItem(time_json, "timezone");
            if (timezone_json != NULL && cJSON_IsString(timezone_json)) {
                strncpy(timezone_str, timezone_json->valuestring, sizeof(timezone_str) - 1);
                timezone_str[sizeof(timezone_str) - 1] = '\0';  // Ensure null-termination
            }
        }

        cJSON_Delete(root);
        free(buffer);
    } else {
        ESP_LOGE(TAG, "Failed to open config file");
    }

    char tz_str[32];
    snprintf(tz_str, sizeof(tz_str), "%s", timezone_str);
    ESP_LOGI(TAG, "tz_str: %s", tz_str);
    setenv("TZ", tz_str, 1);
    ESP_LOGI(TAG, "TZ environment variable set to: %s", getenv("TZ"));
    tzset();
    ESP_LOGI(TAG, "TZ environment variable set to (after tzset): %s", getenv("TZ"));

    char strftime_buf[64];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}
