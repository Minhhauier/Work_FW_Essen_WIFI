#ifndef SETUP_WIFI_H
#define SETUP_WIFI_H
#include <stdbool.h>

extern bool s_connected;
extern const char *html_page_2;
extern int wifi_state;
extern bool act_handle;
void setup_wifi_init(void);
void exit_accesspoint();
void reopen_network();
void try_connect_saved();
void publish_infor_wifi(void);
 void scan_wifi_to_connect();
#endif