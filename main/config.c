#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <cJSON.h>

#include "wifi_prov.h"
#include "config.h"

/*
In this file's comments, we use the term `configuration` to refer to the actual JSON configuration
file found at `CONFIG_FILENAME` and we use the term `config` to refer to the current WIFI
configuration that is in flash.  Function names regardless, use `config` for brevity.
*/

/* FreeRTOS event group to signal when we are connected */
// static EventGroupHandle_t wifi_event_group;

static const char *TAG = "config";

/* Write default configuration */
void write_default_config(const char *ssid, const char *password) {
    FILE* f = fopen(CONFIG_FILENAME, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "    \"ssid\": \"%s\",\n", ssid);
    fprintf(f, "    \"pass\": \"%s\",\n", password);
    fprintf(f, "    \"ntp\": \"pool.ntp.org\",\n");
    fprintf(f, "    \"colon\": \"1\",\n");
    fprintf(f, "    \"time\": {\n");
    fprintf(f, "        \"city\": \"Los Angeles\",\n");
    fprintf(f, "        \"timezone\": \"PST8PDT,M3.2.0,M11.1.0\",\n");
    fprintf(f, "        \"time_fmt\": \"1\"\n");
    fprintf(f, "    },\n");
    fprintf(f, "    \"color\": {\n");
    fprintf(f, "        \"r\": \"0\",\n");
    fprintf(f, "        \"g\": \"0\",\n");
    fprintf(f, "        \"b\": \"0\"\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");

    fclose(f);
    ESP_LOGI(TAG, "Default configuration has been written to %s", CONFIG_FILENAME);
}

/* Read out the configuration data */
char* read_json_data() {
    FILE* f = fopen(CONFIG_FILENAME, "r");
    if (f == NULL) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = (char*)malloc(size + 1);
    if (data == NULL) {
        fclose(f);
        return NULL;
    }

    fread(data, 1, size, f);
    fclose(f);
    data[size] = '\0';

    return data;
}

/* Search for value in JSON object */
bool find_value_in_json(cJSON *obj, const char *key, char *value, size_t value_size) {
    if (obj == NULL || key == NULL || value == NULL || value_size == 0) {
        return false;
    }

    if (cJSON_IsObject(obj)) {
        cJSON *child = obj->child;
        while (child != NULL) {
            if (strcmp(child->string, key) == 0 && cJSON_IsString(child)) {
                strncpy(value, child->valuestring, value_size);
                value[value_size - 1] = '\0';  // Ensure null termination
                return true;
            } else if (cJSON_IsObject(child)) {
                if (find_value_in_json(child, key, value, value_size)) {
                    return true;
                }
            }
            child = child->next;
        }
    }

    return false;
}

/* Read value from configuration file */
void read_config_value(const char *key, char *value, size_t value_size) {
    FILE *file = fopen(CONFIG_FILENAME, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "Failed to open config file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        ESP_LOGI(TAG, "Failed to allocate memory for buffer");
        fclose(file);
        return;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);
    buffer[file_size] = '\0';

    cJSON *root = cJSON_Parse(buffer);
    if (root == NULL) {
        ESP_LOGI(TAG, "Failed to parse JSON");
        free(buffer);
        return;
    }

    if (!find_value_in_json(root, key, value, value_size)) {
        ESP_LOGI(TAG, "%s not found in JSON or is not a string", key);
    }

    cJSON_Delete(root);
    free(buffer);
}

/* Write value to configuration file */
void write_config_value(const char *key, const char *value) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGI(TAG, "Failed to create JSON object");
        return;
    }

    cJSON *value_json = cJSON_CreateString(value);
    if (value_json == NULL) {
        cJSON_Delete(root);
        ESP_LOGI(TAG, "Failed to create JSON string");
        return;
    }

    cJSON_AddItemToObject(root, key, value_json);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (json_str == NULL) {
        ESP_LOGI(TAG, "Failed to generate JSON string");
        return;
    }

    FILE *file = fopen(CONFIG_FILENAME, "w");
    if (file == NULL) {
        ESP_LOGI(TAG, "Failed to open config file for writing");
        free(json_str);
        return;
    }

    fputs(json_str, file);
    fclose(file);
    free(json_str);
    ESP_LOGI(TAG, "Config file updated successfully");
}


// /* Update WIFI config */
// void update_config_wifi(const char *ssid, const char *password, const char *prev_ssid, const char *prev_password) {
//     if (strncmp(ssid, prev_ssid, 32) != 0 || strncmp(password, prev_password, 64) != 0) {
//         ESP_LOGI(TAG, "SSID or password has changed");

//         ESP_ERROR_CHECK(esp_wifi_stop());

//         /* Copy the ssid and password from configuration file to the WIFI config */
//         wifi_config_t current_config;
//         esp_wifi_get_config(ESP_IF_WIFI_STA, &current_config);
//         strncpy((char *)current_config.sta.ssid, ssid, sizeof(current_config.sta.ssid));
//         strncpy((char *)current_config.sta.password, password, sizeof(current_config.sta.password));

//         /* Set new WIFI config and start WIFI */
//         ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &current_config));
//         ESP_ERROR_CHECK(esp_wifi_start());

//         /* Wait for connection */
//         EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
//                                                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                                pdFALSE,
//                                                pdFALSE,
//                                                portMAX_DELAY);
//         if (bits & WIFI_CONNECTED_BIT) {
//             ESP_LOGI(TAG, "Connected to AP SSID: %s", ssid);
//         } else if (bits & WIFI_FAIL_BIT) {
//             ESP_LOGE(TAG, "Failed to connect to AP SSID: %s", ssid);

//             /* Reset the WIFI config to previous settings */
//             ESP_ERROR_CHECK(esp_wifi_stop());
//             strncpy((char *)current_config.sta.ssid, prev_ssid, sizeof(current_config.sta.ssid));
//             strncpy((char *)current_config.sta.password, prev_password, sizeof(current_config.sta.password));
//             ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &current_config));
//             ESP_ERROR_CHECK(esp_wifi_start());

//             /* Update the configuration file with the previous WIFI config */
//             FILE *file = fopen(CONFIG_FILENAME, "r+");
//             if (file != NULL) {
//                 fseek(file, 0, SEEK_END);
//                 long file_size = ftell(file);
//                 fseek(file, 0, SEEK_SET);
//                 char *buffer = (char *)malloc(file_size + 1);
//                 if (buffer != NULL) {
//                     fread(buffer, 1, file_size, file);
//                     buffer[file_size] = '\0';
//                     cJSON *root = cJSON_Parse(buffer);
//                     if (root != NULL) {
//                         cJSON *ssid_json = cJSON_GetObjectItem(root, "ssid");
//                         if (ssid_json != NULL && cJSON_IsString(ssid_json)) {
//                             cJSON_SetValuestring(ssid_json, prev_ssid);
//                         }

//                         cJSON *pass_json = cJSON_GetObjectItem(root, "pass");
//                         if (pass_json != NULL && cJSON_IsString(pass_json)) {
//                             cJSON_SetValuestring(pass_json, prev_password);
//                         }

//                         fseek(file, 0, SEEK_SET);
//                         fputs(cJSON_Print(root), file);
//                         cJSON_Delete(root);
//                     } else {
//                         ESP_LOGE(TAG, "Failed to parse JSON");
//                     }
//                     free(buffer);
//                 } else {
//                     ESP_LOGE(TAG, "Failed to allocate memory for buffer");
//                 }
//                 fclose(file);
//             } else {
//                 ESP_LOGE(TAG, "Failed to open config file");
//             }
//         } else {
//             ESP_LOGE(TAG, "Unexpected event occurred");
//         }
//     } else {
//         ESP_LOGI(TAG, "SSID and password match");
//     }
// }

/* Check current WIFI configuration with current WIFI config */
void check_and_update_wifi_config(wifi_config_t *current_config) {

    char ssid[32];
    char password[64];
    char prev_ssid[32];
    char prev_password[64];

    /* Read WIFI configuration */
    read_config_value("ssid", ssid, sizeof(ssid));
    read_config_value("pass", password, sizeof(password));

    // /* Get current flashed WIFI config */
    // wifi_config_t current_config;
    // esp_wifi_get_config(ESP_IF_WIFI_STA, &current_config);

    /* Copy current SSID and password to previous config variables */
    strncpy(prev_ssid, (char *)current_config->sta.ssid, sizeof(prev_ssid));
    strncpy(prev_password, (char *)current_config->sta.password, sizeof(prev_password));

    ESP_LOGI(TAG, "Previous SSID: %s", prev_ssid);
    ESP_LOGI(TAG, "Previous password: %s", prev_password);

    char* data = read_json_data();
    if (data == NULL) {
        printf("Error: Failed to read JSON data.\n");
    } else {
        printf("JSON data: %s\n", data);
        free(data);
    }

    /* Update WIFI configuration */
    if ((strncmp(ssid, prev_ssid, 32) != 0 || strncmp(password, prev_password, 64) != 0) &&
        (ssid[0] != '\0' && password[0] != '\0')) {
        ESP_LOGI(TAG, "SSID or password has changed");
        strncpy((char *)current_config->sta.ssid, ssid, sizeof(current_config->sta.ssid));
        strncpy((char *)current_config->sta.password, password, sizeof(current_config->sta.password));
        /* Set new WIFI config and start WIFI */
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, current_config));
    }
    // update_config_wifi(ssid, password, prev_ssid, prev_password);
}

void config_init(void) {

    /* Get current WiFi config */
    wifi_config_t current_config;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &current_config);

    /* Create default config file if it doesn't exist */
    struct stat st;
    if (stat(CONFIG_FILENAME, &st) != 0) {
        write_default_config((char *)current_config.sta.ssid, (char *)current_config.sta.password);
    } else {
        ESP_LOGI(TAG, "Default configuration file already exists!");
        /* Check and update WIFI configuration */
        check_and_update_wifi_config(&current_config);
    }
}
