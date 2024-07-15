#ifndef WSSERVER_H
#define WSSERVER_H

#include <esp_http_server.h>

void configure_leds(void);

esp_err_t start_webserver(void);

#endif /* WSSERVER_H */
