#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_https_ota.h>
#include "esp_ota_ops.h"
#include "esp_http_client.h"

#define TAG "OTA"

static int total_bytes = 0;
static int received_bytes = 0;
static int last_percent = 0; 
bool on_ota = false;
// static void event_handler(void* arg, esp_event_base_t event_base,
//                         int32_t event_id, void* event_data)
// {
//     if (event_base == ESP_HTTPS_OTA_EVENT) {
//         switch (event_id) {
//             case ESP_HTTPS_OTA_START:
//                 ESP_LOGI(TAG, "OTA started");
//                 break;
//             case ESP_HTTPS_OTA_CONNECTED:
//                 ESP_LOGI(TAG, "Connected to server");
//                 break;
//             case ESP_HTTPS_OTA_GET_IMG_DESC:
//                 ESP_LOGI(TAG, "Reading Image Description");
//                 break;
//             case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
//                 ESP_LOGI(TAG, "Verifying chip id of new image: %d", *(esp_chip_id_t *)event_data);
//                 break;
//             case ESP_HTTPS_OTA_DECRYPT_CB:
//                 ESP_LOGI(TAG, "Callback to decrypt function");
//                 break;
//             case ESP_HTTPS_OTA_WRITE_FLASH:
//                 ESP_LOGD(TAG, "Writing to flash: %d written", *(int *)event_data);
//                 break;
//             case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
//                 ESP_LOGI(TAG, "Boot partition updated. Next Partition: %d", *(esp_partition_subtype_t *)event_data);
//                 break;
//             case ESP_HTTPS_OTA_FINISH:
//                 ESP_LOGI(TAG, "OTA finish");
//                 break;
//             case ESP_HTTPS_OTA_ABORT:
//                 ESP_LOGI(TAG, "OTA abort");
//                 break;
//         }
//     }
// }

esp_err_t do_firmware_upgrade(char *bin_url)
{
    // ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    if(bin_url==NULL) return ESP_OK;
    on_ota=true;
    esp_http_client_config_t config = {
        .url = bin_url,
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,  
        .timeout_ms = 50000,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
        .http_client_init_cb = NULL,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful, restarting....");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed, restarting....");
        esp_restart();
        return ESP_FAIL;
    }
    on_ota=false;
    return ESP_OK;
}
