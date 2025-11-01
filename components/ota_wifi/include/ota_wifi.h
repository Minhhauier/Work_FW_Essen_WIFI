#ifndef OTA_WIFI_H
#define OTA_WIFI_H

#include <esp_err.h>


extern bool on_ota;
esp_err_t do_firmware_upgrade(char *bin_url);

#endif
