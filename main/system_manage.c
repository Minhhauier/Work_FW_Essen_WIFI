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
    while (s_connected == false)
    {
        if(act_handle==false && mqtt_connected==false){
            try_connect_saved();
            printf("Try connect wifi saved\r\n");
            //count=0;
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "WiFi connected, starting MQTT");
    mqtt_start();
    while (1)
    {
        if(act_handle==false && mqtt_connected==false){
            try_connect_saved();
            printf("Try connect wifi saved\r\n");
            //count=0;
        }
        //else printf("setup wifi\r\n");
        // if(xQueueReceive(mqtt_queue_handle, data_receive,portMAX_DELAY) == pdTRUE) {
        //    // ESP_LOGI(TAG, "Data received from MQTT queue: %s", data_receive);
        //     convert_to_json(data_receive);
        // }
        // if(xQueueReceive(publish_queue_handle, data_receive, portMAX_DELAY) == pdTRUE) {
        //     // Process publish queue item
        //     //ESP_LOGI(TAG, "Data received from publish queue: %s", data_receive);
        //     mqtt_publish_data(data_receive, topic);
        //     // Handle publish data here
        // }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}