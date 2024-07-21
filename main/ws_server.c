#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <led_strip.h>
#include <sdkconfig.h>
#include <cJSON.h>

#include "esp_heap_caps.h"
#include "vfs.h"
#include "leds.h"
#include "config.h"
#include "ws_server.h"


static const char *TAG = "server";

extern led_strip_handle_t led_strip;

static const char html_header[] =
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1, user-scalable=0\">"
    "<title>Wi-Fi Nixie Clock</title>"
    "<style>";

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_sendstr_chunk(req, html_header);

    /* Get embedded styles.css file */
    extern const unsigned char css_start[] asm("_binary_styles_css_start");
    extern const unsigned char css_end[]   asm("_binary_styles_css_end");
    const size_t css_size = (css_end - css_start);
    httpd_resp_send_chunk(req, (const char *)css_start, css_size);

    httpd_resp_sendstr_chunk(req, "</style><script>");

    /* Get embedded iro.min.js file */
    extern const unsigned char iro_start[] asm("_binary_iro_min_js_start");
    extern const unsigned char iro_end[]   asm("_binary_iro_min_js_end");
    const size_t iro_size = (iro_end - iro_start);
    httpd_resp_send_chunk(req, (const char *)iro_start, iro_size);

    httpd_resp_sendstr_chunk(req, "</script><script>");

    /* Get embedded settings.js file */
    extern const unsigned char settings_start[] asm("_binary_settings_js_start");
    extern const unsigned char settings_end[]   asm("_binary_settings_js_end");
    const size_t settings_size = (settings_end - settings_start);
    httpd_resp_send_chunk(req, (const char *)settings_start, settings_size);

    httpd_resp_sendstr_chunk(req, "</script></head>");

    /* Get embedded body.html file */
    extern const unsigned char body_start[] asm("_binary_body_html_start");
    extern const unsigned char body_end[]   asm("_binary_body_html_end");
    const size_t body_size = (body_end - body_start);
    httpd_resp_send_chunk(req, (const char *)body_start, body_size);

    httpd_resp_sendstr_chunk(req, "</html>");

    // /* Get embedded html file */
    // extern const unsigned char config_start[] asm("_binary_config_html_start");
    // extern const unsigned char config_end[]   asm("_binary_config_html_end");
    // const size_t config_size = (config_end - config_start);
    // httpd_resp_send_chunk(req, (const char *)config_start, config_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static esp_err_t color_picker_handler(httpd_req_t *req)
{
    char *red_pos = strstr(req->uri, "red=");
    char *green_pos = strstr(req->uri, "green=");
    char *blue_pos = strstr(req->uri, "blue=");

    if (red_pos && green_pos && blue_pos) {
        /* Parse the integers for red, green, and blue
           while skipping "blue=" etc. and parse integer */
        int red, green, blue;
        sscanf(red_pos + 4, "%d", &red);
        sscanf(green_pos + 6, "%d", &green);
        sscanf(blue_pos + 5, "%d", &blue);

        for (int i=0; i < 6; i++) {
            led_strip_set_pixel(led_strip, i, red, green, blue);
            led_strip_refresh(led_strip); // refresh to send the data
        }
    } else {
        ESP_LOGI(TAG, "Invalid URL format");
        /* Set all LEDs off to clear all pixels */
        led_strip_clear(led_strip);
    }

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static esp_err_t jSON_post_handler(httpd_req_t *req) {
    if (!heap_caps_check_integrity_all(true)) {
        ESP_LOGI(TAG, "INTEGRITY ISSUES!");
    }
    /* Receive JSON data from the client and update configuration file. */
    char buf[1024]; // Adjust the buffer size according to your JSON data size
    int total_len = req->content_len;
    int remaining_len = total_len;

    char *data = (char *)malloc(total_len);
    if (!data) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int received = 0;
    while (remaining_len > 0) {
        int ret = httpd_req_recv(req, buf, MIN(remaining_len, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
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
    free(data);  // Free data after parsing JSON
    if (json == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    FILE* f = fopen(CONFIG_FILENAME, "w");
    if (f == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char* jsonString = cJSON_Print(json);
    if (jsonString == NULL) {
        cJSON_Delete(json);
        fclose(f);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    fwrite(jsonString, 1, strlen(jsonString), f);
    fclose(f);
    free(jsonString);

    cJSON_Delete(json);

    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t jSON_get_handler(httpd_req_t *req) {
    /* Get JSON config file data. */
    char* data = read_json_data();
    if (data == NULL) {
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    cJSON* json = cJSON_Parse(data);
    free(data);
    if (json == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char* jsonString = cJSON_Print(json);
    cJSON_Delete(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, jsonString, strlen(jsonString));
    free(jsonString);

    return ESP_OK;
}

static esp_err_t jSON_reboot_handler(httpd_req_t *req) {
    /* Send a response to indicate that everything is okay */
    const char* response_data = "System will reboot now";
    httpd_resp_send(req, response_data, strlen(response_data));

    /* Unregister vfs */
    vfs_unregister();

    /* Delay for a second before rebooting */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Turn off the LEDs */
    led_strip_set_pixel(led_strip, 0, 0, 0, 0);
    led_strip_refresh(led_strip); // refresh to send the data
    led_strip_clear(led_strip);

    /* Reboot the system */
    esp_restart();

    return ESP_OK;
}

static const httpd_uri_t favicon = {
        .uri        = "/favicon.ico",
        .method     = HTTP_GET,
        .handler    = favicon_get_handler,
        .user_ctx   = NULL
};

static const httpd_uri_t root = {
        .uri        = "/",
        .method     = HTTP_GET,
        .handler    = root_get_handler,
        .user_ctx   = NULL
};

static const httpd_uri_t picker = {
        .uri        = "/rgb",
        .method     = HTTP_GET,
        .handler    = color_picker_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static const httpd_uri_t update = {
        .uri        = "/update",
        .method     = HTTP_POST,
        .handler    = jSON_post_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static const httpd_uri_t data = {
        .uri        = "/data",
        .method     = HTTP_GET,
        .handler    = jSON_get_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static const httpd_uri_t reboot = {
        .uri        = "/reboot",
        .method     = HTTP_POST,
        .handler    = jSON_reboot_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};


esp_err_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &picker);
        httpd_register_uri_handler(server, &update);
        httpd_register_uri_handler(server, &data);
        httpd_register_uri_handler(server, &reboot);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return ESP_FAIL;
}
