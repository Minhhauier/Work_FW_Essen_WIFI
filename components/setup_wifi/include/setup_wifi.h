#ifndef SETUP_WIFI_H
#define SETUP_WIFI_H

extern bool s_connected;
// #include "esp_http_server.h"
extern const char* html_page;
void setup_wifi_init(void);
void try_connect_saved();
//httpd_handle_t start_webserver(void);
void open_webserver();
#endif