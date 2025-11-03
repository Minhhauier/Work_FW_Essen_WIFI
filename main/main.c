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
#include "control_relay.h"
#include "gpio_cf.h"
#include "ota_wifi.h"
#include "pzem.h"


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
    setup_wifi_init();
    all_led_by_status(0);
    config_gpio_detect_zero();
    config_gpio_wifi_menu_config();
    config_gpio_led();
    while (s_connected == false)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "WiFi connected, starting MQTT");
    mqtt_start();
    
    xTaskCreate(&task_system_manage, "system_manage_task", 1024*4, NULL, 5, NULL);
    // xTaskCreate(&pzem_task,"pzem task", 1024*4, NULL, 5, NULL);
    do_firmware_upgrade(NULL);

    while (1)
    {
        if(gpio_get_level(GPIO_WIFI_CONFIG)==0)
        {
            while (gpio_get_level(GPIO_WIFI_CONFIG)==0)
            {
                vTaskDelay(100/portTICK_PERIOD_MS);
            }
            printf("Open menuconfig\r\n");
            open_webserver();
            // start_stop_timer();
        }
        // if(state_mqtt==3){
        //     gpio_set_level(GPIO_WIFI_CONFIG,0);
        //     vTaskDelay(500/portTICK_PERIOD_MS);
        //     gpio_set_level(GPIO_WIFI_CONFIG,1);
        //     vTaskDelay(500/portTICK_PERIOD_MS);
        // }
        //ESP_LOGI(TAG, "Main task running...");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}
