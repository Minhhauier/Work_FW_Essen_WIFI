#ifndef SETUP_WIFI_H
#define SETUP_WIFI_H
#include <stdbool.h>

extern bool s_connected;
extern const char *html_page_1;
extern int wifi_state;
void setup_wifi_init(void);
void exit_accesspoint();
void reopen_network();
void try_connect_saved();

#endif