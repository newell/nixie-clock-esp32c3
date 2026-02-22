#include "config.h"

#include <cJSON.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "wifi_prov.h"

/* In this file's comments, we use the term `configuration` to refer to the
actual JSON configuration file found at `CONFIG_FILENAME` and we use the term
`config` to refer to the current WIFI configuration that is in flash.  Function
names regardless, use `config` for brevity. */

static const char* TAG = "config";

void write_default_config(void) {
    FILE* f = fopen(CONFIG_FILENAME, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "    \"ssid\": \"\",\n");
    fprintf(f, "    \"pass\": \"\",\n");
    fprintf(f, "    \"ntp\": \"pool.ntp.org\",\n");
    fprintf(f, "    \"colon\": \"1\",\n");
    fprintf(f, "    \"time\": {\n");
    fprintf(f, "        \"city\": \"Los Angeles\",\n");
    fprintf(f, "        \"timezone\": \"PST8PDT,M3.2.0,M11.1.0\",\n");
    fprintf(f, "        \"time_fmt\": \"1\"\n");
    fprintf(f, "    },\n");
    fprintf(f, "    \"led_mode\": \"static\",\n");
    fprintf(f, "    \"color\": {\n");
    fprintf(f, "        \"r\": \"0\",\n");
    fprintf(f, "        \"g\": \"0\",\n");
    fprintf(f, "        \"b\": \"0\"\n");
    fprintf(f, "    }\n");
    fprintf(f, "}\n");

    fclose(f);
    ESP_LOGI(TAG, "Default configuration has been written to %s",
             CONFIG_FILENAME);
}

char* read_json_data() {
    FILE* f = fopen(CONFIG_FILENAME, "r");
    if (f == NULL) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size == -1L) {
        fclose(f);
        return NULL;
    }

    fseek(f, 0, SEEK_SET);
    char* data = (char*)malloc(size + 1);
    if (data == NULL) {
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(data, 1, size, f);
    if (read_size != size) {
        free(data);
        fclose(f);
        return NULL;
    }

    fclose(f);
    data[size] = '\0';
    return data;
}

bool find_value_in_json(cJSON* obj, const char* key, char* value,
                        size_t value_size) {
    if (obj == NULL || key == NULL || value == NULL || value_size == 0) {
        return false;
    }

    if (cJSON_IsObject(obj)) {
        cJSON* child = obj->child;
        while (child != NULL) {
            if (strcmp(child->string, key) == 0 && cJSON_IsString(child)) {
                strncpy(value, child->valuestring, value_size);
                value[value_size - 1] = '\0';
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

void read_config_value(const char* key, char* value, size_t value_size) {
    FILE* file = fopen(CONFIG_FILENAME, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "Failed to open config file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size == -1L) {
        ESP_LOGI(TAG, "Failed to determine file size");
        fclose(file);
        return;
    }

    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        ESP_LOGI(TAG, "Failed to allocate memory for buffer");
        fclose(file);
        return;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);
    if (read_size != file_size) {
        ESP_LOGI(TAG, "Failed to read the entire file");
        free(buffer);
        return;
    }

    buffer[file_size] = '\0';
    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (root == NULL) {
        ESP_LOGI(TAG, "Failed to parse JSON");
        return;
    }

    if (!find_value_in_json(root, key, value, value_size)) {
        ESP_LOGI(TAG, "%s not found in JSON or is not a string", key);
    }

    cJSON_Delete(root);
}

void write_config_value(const char* key, const char* value) {
    FILE* file = fopen(CONFIG_FILENAME, "r");
    cJSON* root = NULL;

    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        if (length == -1L) {
            ESP_LOGI(TAG, "Failed to determine file size");
            fclose(file);
            return;
        }

        fseek(file, 0, SEEK_SET);
        char* data = (char*)malloc(length + 1);
        if (data == NULL) {
            ESP_LOGI(TAG, "Failed to allocate memory for data");
            fclose(file);
            return;
        }

        size_t read_size = fread(data, 1, length, file);
        fclose(file);
        if (read_size != length) {
            ESP_LOGI(TAG, "Failed to read the entire file");
            free(data);
            return;
        }

        data[length] = '\0';
        root = cJSON_Parse(data);
        free(data);
        if (root == NULL) {
            ESP_LOGI(TAG, "Failed to parse JSON");
            return;
        }
    } else {
        root = cJSON_CreateObject();
    }

    cJSON* value_json = cJSON_CreateString(value);
    if (value_json == NULL) {
        cJSON_Delete(root);
        ESP_LOGI(TAG, "Failed to create JSON string");
        return;
    }

    cJSON_ReplaceItemInObject(root, key, value_json);

    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (json_str == NULL) {
        ESP_LOGI(TAG, "Failed to generate JSON string");
        return;
    }

    file = fopen(CONFIG_FILENAME, "w");
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

void config_init(void) {
    // // Get current WiFi config
    // wifi_config_t current_config;
    // esp_wifi_get_config(ESP_IF_WIFI_STA, &current_config);

    /* Create default config file if it doesn't exist */
    struct stat st;
    if (stat(CONFIG_FILENAME, &st) != 0) {
        write_default_config();
    } else {
        ESP_LOGI(TAG, "Default configuration file already exists!");
    }
}
