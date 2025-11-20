#include <stdio.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_system.h>
#include <esp_mac.h>

#include "setup_wifi.h"
#include "mqtt_wifi.h"
#include "system_manage.h"
#include "control_led.h"
#include "gpio_cf.h"
#include "ota_wifi.h"
#include "pzem.h"
#include "uart.h"
#include "config_parameter.h"
#include "encrypt_decrypt.h"

#define TAG "MAIN"
char device_name[25];

void get_device_name(char *device_name) {
    uint8_t mac[6];
    esp_err_t res = esp_efuse_mac_get_default(mac);
    if (res == ESP_OK) {
        snprintf(device_name, 25, "EV%02x%02x%02x%02x%02x",
                 mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to read MAC address");
    }
}
void app_main(void)
{   
    get_device_name(device_name);
    ESP_LOGI(TAG,"==Device name: %s==",device_name);
    ESP_LOGI(TAG, "Starting setup_wifi_init()");
    config_gpio_detect_zero();
    all_led_by_status(0);
    config_gpio_led();
    configure_uart_dynamic_Pzem(UART_PZEM_NUM, 9600, TX_PZEM, RX_PZEM);
    xTaskCreate(&task_system_manage, "system_manage_task", 1024*8, NULL, 10, NULL);
    xTaskCreate(&pzem_task,"pzem task", 1024*4, NULL, 10, NULL);
    xTaskCreate(&detect_wifi_task,"detect wifi task",1024*4,NULL,10,NULL);
    //fixed can't reference to ...
    do_firmware_upgrade(NULL);
    convert_to_json(NULL);
    while (1)
    {
        //ESP_LOGI(TAG, "Main task running...");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
