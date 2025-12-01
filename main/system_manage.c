#include <stdio.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "system_manage.h"
#include "config_parameter.h"
#include "encrypt_decrypt.h"
#include "mqtt_wifi.h"
#include "setup_wifi.h"

#define TAG "SYSTEM_MANAGE"

QueueHandle_t mqtt_queue_handle;
QueueHandle_t gps_queue_handle;
QueueHandle_t publish_queue_handle;

static char data_receive[BUF_SIZE_MQTT];

void task_system_manage(void *pvParameter)
{
    mqtt_queue_handle = xQueueCreate(10, BUF_SIZE_MQTT);
    gps_queue_handle = xQueueCreate(10, BUF_SIZE_MQTT);
    publish_queue_handle = xQueueCreate(10, BUF_SIZE_MQTT);
    static char topic[64];
    snprintf(topic, sizeof(topic), "%s/SmartEVsafe", PUB);
    setup_wifi_init();
    // wifi_state=2;
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    while (got_ip == false)
    {
        if(act_handle==false && mqtt_connected==false){
            if(!got_ip){
                printf("==Try connect saved wifi\r\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                // if(got_ip)  break;
                scan_wifi_to_connect();
            }
            //try_connect_saved();
            //count=0;
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "WiFi connected, starting MQTT");
    mqtt_start();
    while (1)
    {
        if(act_handle==false && mqtt_connected==false){
            //try_connect_saved();
            if(!got_ip)
            {
                printf("==Try connect saved wifi\r\n");
                scan_wifi_to_connect();
            }
            //count=0;
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}