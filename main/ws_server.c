#include "ws_server.h"

#include <cJSON.h>
#include <driver/gpio.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "clock.h"
#include "config.h"
#include "esp_heap_caps.h"
#include "leds.h"
#include "vfs.h"

static const char* TAG = "server";

static const char html_header[] =
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, "
    "maximum-scale=1, user-scalable=0\">"
    "<title>Wi-Fi Nixie Clock</title>"
    "<style>";

static esp_err_t favicon_get_handler(httpd_req_t* req) {
    extern const unsigned char favicon_ico_start[] asm(
        "_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char*)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t* req) {
    httpd_resp_sendstr_chunk(req, html_header);
    extern const unsigned char css_start[] asm("_binary_styles_css_start");
    extern const unsigned char css_end[] asm("_binary_styles_css_end");
    httpd_resp_send_chunk(req, (const char*)css_start, (css_end - css_start));

    httpd_resp_sendstr_chunk(req, "</style><script>");
    extern const unsigned char iro_start[] asm("_binary_iro_min_js_start");
    extern const unsigned char iro_end[] asm("_binary_iro_min_js_end");
    httpd_resp_send_chunk(req, (const char*)iro_start, (iro_end - iro_start));

    httpd_resp_sendstr_chunk(req, "</script><script>");
    extern const unsigned char settings_start[] asm(
        "_binary_settings_js_start");
    extern const unsigned char settings_end[] asm("_binary_settings_js_end");
    httpd_resp_send_chunk(req, (const char*)settings_start,
                          (settings_end - settings_start));

    httpd_resp_sendstr_chunk(req, "</script></head>");
    extern const unsigned char body_start[] asm("_binary_body_html_start");
    extern const unsigned char body_end[] asm("_binary_body_html_end");
    httpd_resp_send_chunk(req, (const char*)body_start,
                          (body_end - body_start));

    httpd_resp_sendstr_chunk(req, "</html>");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

/* Updated Color Picker Handler */
static esp_err_t color_picker_handler(httpd_req_t* req) {
    char* red_pos = strstr(req->uri, "red=");
    char* green_pos = strstr(req->uri, "green=");
    char* blue_pos = strstr(req->uri, "blue=");

    if (red_pos && green_pos && blue_pos) {
        int red, green, blue;
        sscanf(red_pos + 4, "%d", &red);
        sscanf(green_pos + 6, "%d", &green);
        sscanf(blue_pos + 5, "%d", &blue);

        // Update RAM variables
        led_set_ram_color((uint8_t)red, (uint8_t)green, (uint8_t)blue);

        // Send message to LED task instead of direct hardware call
        led_send_msg(LED_CMD_SET_COLOR, (uint8_t)red, (uint8_t)green,
                     (uint8_t)blue);
    } else {
        ESP_LOGI(TAG, "Invalid URL format or missing params");
        led_send_msg(LED_CMD_SET_COLOR, 0, 0, 0);  // Turn off
    }

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t jSON_post_handler(httpd_req_t* req) {
    char buf[256];
    int total_len = req->content_len;
    int remaining_len = total_len;
    char* data = (char*)malloc(total_len + 1);
    if (!data) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int received = 0;
    while (remaining_len > 0) {
        int ret = httpd_req_recv(req, buf, MIN(remaining_len, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            free(data);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        memcpy(data + received, buf, ret);
        remaining_len -= ret;
        received += ret;
    }
    data[received] = '\0';

    cJSON* json = cJSON_Parse(data);
    if (json == NULL) {
        free(data);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // --- RAM SYNC START ---
    // Perform this BEFORE opening the file to avoid any I/O lag
    cJSON* time_fmt = cJSON_GetObjectItem(json, "time_fmt");
    if (cJSON_IsString(time_fmt)) {
        clock_set_ram_format(atoi(time_fmt->valuestring));
    }
    cJSON* mode_item = cJSON_GetObjectItem(json, "led_mode");
    if (cJSON_IsString(mode_item)) {
        led_set_ram_mode(mode_item->valuestring);
    }

    cJSON* color = cJSON_GetObjectItem(json, "color");
    if (color) {
        int r = cJSON_GetObjectItem(color, "r")->valueint;
        int g = cJSON_GetObjectItem(color, "g")->valueint;
        int b = cJSON_GetObjectItem(color, "b")->valueint;
        led_set_ram_color((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }
    // --- RAM SYNC END ---

    // Now handle the persistent storage
    FILE* f = fopen(CONFIG_FILENAME, "w");
    if (f == NULL) {
        cJSON_Delete(json);
        free(data);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char* jsonString = cJSON_Print(json);
    if (jsonString) {
        fwrite(jsonString, 1, strlen(jsonString), f);
        free(jsonString);
    }

    fclose(f);  // Close the file as soon as possible
    cJSON_Delete(json);
    free(data);

    // Final kick to ensure hardware is in sync with the new RAM values
    led_send_msg(LED_CMD_RELOAD_CONFIG, 0, 0, 0);

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t jSON_get_handler(httpd_req_t* req) {
    // Read the raw JSON string from the Flash/NVS
    char* data = read_json_data();
    if (data == NULL) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    // Parse the data to update RAM variables
    // This ensures RAM "snaps back" to Disk state on browser refresh
    cJSON* json = cJSON_Parse(data);
    if (json) {
        // Sync LED Mode
        cJSON* mode_item = cJSON_GetObjectItem(json, "led_mode");
        if (cJSON_IsString(mode_item)) {
            led_set_ram_mode(mode_item->valuestring);
        }

        // Sync RGB Colors
        cJSON* color = cJSON_GetObjectItem(json, "color");
        if (color) {
            cJSON* r = cJSON_GetObjectItem(color, "r");
            cJSON* g = cJSON_GetObjectItem(color, "g");
            cJSON* b = cJSON_GetObjectItem(color, "b");
            if (r && g && b) {
                led_set_ram_color((uint8_t)r->valueint, (uint8_t)g->valueint,
                                  (uint8_t)b->valueint);
            }
        }

        // Sync Clock Time Format (12h/24h)
        cJSON* time_fmt = cJSON_GetObjectItem(json, "time_fmt");
        if (cJSON_IsString(time_fmt)) {
            clock_set_ram_format(atoi(time_fmt->valuestring));
        }

        cJSON_Delete(json);

        // Final kick to the LED task to apply the colors we just synced
        led_send_msg(LED_CMD_RELOAD_CONFIG, 0, 0, 0);
    }

    // Send the JSON to the browser UI
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, data, strlen(data));

    // Clean up the allocated string buffer
    free(data);
    return ESP_OK;
}

static esp_err_t jSON_reboot_handler(httpd_req_t* req) {
    httpd_resp_send(req, "Rebooting...", 12);
    vfs_unregister();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Clear LEDs via the queue before restarting
    led_send_msg(LED_CMD_SET_COLOR, 0, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_restart();
    return ESP_OK;
}

static esp_err_t led_mode_handler(httpd_req_t* req) {
    char* buf;
    size_t buf_len;

    // Get the query string length
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);

            char param[32];
            // Now extract the "mode" from the query string specifically
            if (httpd_query_key_value(buf, "mode", param, sizeof(param)) ==
                ESP_OK) {
                ESP_LOGI(TAG, "Mode parsed: %s", param);
                led_set_ram_mode(param);
            }
        }
        free(buf);
    } else {
        ESP_LOGW(TAG, "No query string found in URI: %s", req->uri);
    }

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t favicon = {
    .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get_handler};
static const httpd_uri_t root = {
    .uri = "/", .method = HTTP_GET, .handler = root_get_handler};
static const httpd_uri_t picker = {
    .uri = "/rgb", .method = HTTP_GET, .handler = color_picker_handler};
static const httpd_uri_t update = {
    .uri = "/update", .method = HTTP_POST, .handler = jSON_post_handler};
static const httpd_uri_t data_uri = {
    .uri = "/data", .method = HTTP_GET, .handler = jSON_get_handler};
static const httpd_uri_t reboot = {
    .uri = "/reboot", .method = HTTP_POST, .handler = jSON_reboot_handler};
static const httpd_uri_t mode_uri = {
    .uri = "/led_mode", .method = HTTP_GET, .handler = led_mode_handler};

esp_err_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &picker);
        httpd_register_uri_handler(server, &update);
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &reboot);
        httpd_register_uri_handler(server, &mode_uri);
        return ESP_OK;
    }
    return ESP_FAIL;
}
