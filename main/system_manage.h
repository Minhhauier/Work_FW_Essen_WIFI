#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include <mqtt_client.h>

extern char device_name[25];
extern QueueHandle_t mqtt_queue_handle;
extern QueueHandle_t gps_queue_handle;
extern QueueHandle_t publish_queue_handle;
void task_system_manage(void *pvParameter);

#endif 