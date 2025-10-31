#include <cJSON.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include <driver/uart.h>
#include <driver/gpio.h>


#include "system_manage.h"
#include "config_parameter.h"
#include "control_led.h"
#include "control_relay.h"
#include "gpio_cf.h"

#define GATE_STATE_TAG "GATE_STATE"
int control_signal=0;

// void ota_task(char *link){
//    send_at_get_respond("AT\r\n",1000);
// //   OTA_from_link(link);
//    esp_restart();
//     while(1)
//     {
//         vTaskDelay(3000/portTICK_PERIOD_MS);
//     }
// }
void parse_json(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        printf("Error parse JSON!\n");
        return;
    }
     int detect_gate = 0;
    const cJSON *serial_number = cJSON_GetObjectItemCaseSensitive(root, "serial_number");
    const cJSON *command_type = cJSON_GetObjectItem(root, "command_type");
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(root, "data");
    const cJSON *data_new = cJSON_GetObjectItemCaseSensitive(data,"data");
    if (cJSON_IsString(serial_number)) {
        char *ser_num = serial_number->valuestring;
        if (strcmp(ser_num, device_name) == 0) {
            if (cJSON_IsNumber(command_type)) {
                int cmd_type = command_type->valueint;
                if (cmd_type == 101) {
                    const cJSON *gun1 = cJSON_GetObjectItemCaseSensitive(data, "gun1");
                    const cJSON *gun2 = cJSON_GetObjectItemCaseSensitive(data, "gun2");
                    const cJSON *gun3 = cJSON_GetObjectItemCaseSensitive(data, "gun3");
                    const cJSON *gun4 = cJSON_GetObjectItemCaseSensitive(data, "gun4");
                    const cJSON *gun5 = cJSON_GetObjectItemCaseSensitive(data, "gun5");
                    const cJSON *gun6 = cJSON_GetObjectItemCaseSensitive(data, "gun6");
                    // const cJSON *gun1 = cJSON_GetObjectItemCaseSensitive(data_new, "gun1");
                    // const cJSON *gun2 = cJSON_GetObjectItemCaseSensitive(data_new, "gun2");
                    // const cJSON *gun3 = cJSON_GetObjectItemCaseSensitive(data_new, "gun3");
                    // const cJSON *gun4 = cJSON_GetObjectItemCaseSensitive(data_new, "gun4");
                    // const cJSON *gun5 = cJSON_GetObjectItemCaseSensitive(data_new, "gun5");
                    // const cJSON *gun6 = cJSON_GetObjectItemCaseSensitive(data_new, "gun6");
                    const cJSON *V = cJSON_GetObjectItemCaseSensitive(data_new, "V");
                    if (gun1 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun1, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun1, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun1, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 1: Stop");
                                charging_led_by_status(GATE_NUM_1, LED_READY);
                                set_gate_state(GATE_1, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 1: Charging");
                                charging_led_by_status(GATE_NUM_1, LED_CHARGING);
                                set_gate_state(GATE_1, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act == 3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 1: Waiting");
                                charging_led_by_status(GATE_NUM_1, LED_WAITTING);
                                set_gate_state(GATE_1, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                          //  printf("Gun1 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                           // printf("Gun1 limit_time: %d\r\n", lmt);
                        }
                    }
                     if (gun2 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun2, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun2, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun2, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 2: Stop");
                                charging_led_by_status(GATE_NUM_2, LED_READY);
                                set_gate_state(GATE_2, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 2: Charging");
                                charging_led_by_status(GATE_NUM_2, LED_CHARGING);
                                set_gate_state(GATE_2, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act == 3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 2: Waiting");
                                charging_led_by_status(GATE_NUM_2, LED_WAITTING);
                                set_gate_state(GATE_2, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                            //printf("Gun2 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                            //printf("Gun2 limit_time: %d\r\n", lmt);
                        }
                    }
                    if (gun3 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun3, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun3, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun3, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 3: Stop");
                                charging_led_by_status(GATE_NUM_3, LED_READY);
                                set_gate_state(GATE_3, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 3: Charging");
                                charging_led_by_status(GATE_NUM_3, LED_CHARGING);
                                set_gate_state(GATE_3, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act == 3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 3: Waiting");
                                charging_led_by_status(GATE_NUM_3, LED_WAITTING);
                                set_gate_state(GATE_3, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                            //printf("Gun3 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                            //printf("Gun3 limit_time: %d\r\n", lmt);
                        }
                    }
                     if (gun4 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun4, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun4, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun4, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 4: Stop");
                                charging_led_by_status(GATE_NUM_4, LED_READY);
                                set_gate_state(GATE_4, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 4: Charging");
                                charging_led_by_status(GATE_NUM_4, LED_CHARGING);
                                set_gate_state(GATE_4, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act == 3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 4: Waiting");
                                charging_led_by_status(GATE_NUM_4, LED_WAITTING);
                                set_gate_state(GATE_4, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                            //printf("Gun4 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                            //printf("Gun4 limit_time: %d\r\n", lmt);
                        }
                    }
                     if (gun5 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun5, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun5, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun5, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 5: Stop");
                                charging_led_by_status(GATE_NUM_5, LED_READY);
                                set_gate_state(GATE_5, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 5: Charging");
                                charging_led_by_status(GATE_NUM_5, LED_CHARGING);
                                set_gate_state(GATE_5, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act == 3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 5: Waiting");
                                charging_led_by_status(GATE_NUM_5, LED_WAITTING);
                                set_gate_state(GATE_5, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                            //printf("Gun5 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                            //printf("Gun1 limit_time: %d\r\n", lmt);
                        }
                    }
                     if (gun6 != NULL) {
                        const cJSON *action = cJSON_GetObjectItemCaseSensitive(gun6, "action");
                        const cJSON *order_time = cJSON_GetObjectItemCaseSensitive(gun6, "order_time");
                        const cJSON *limit_time = cJSON_GetObjectItemCaseSensitive(gun6, "limit_time");
                        if (cJSON_IsNumber(action)) {
                            int act = action->valueint;
                            if (act == 2) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 6: Stop");
                                charging_led_by_status(GATE_NUM_6, LED_READY);
                                set_gate_state(GATE_6, GATE_DISCHARGE);
                            } else if (act == 1) {
                                ESP_LOGI(GATE_STATE_TAG, "Gate 6: Charging");
                                charging_led_by_status(GATE_NUM_6, LED_CHARGING);
                                set_gate_state(GATE_6, GATE_CHARGE);
                                detect_gate=1;
                                control_signal=1;
                            }
                            else if (act==3){
                                ESP_LOGI(GATE_STATE_TAG, "Gate 6: Waiting");
                                charging_led_by_status(GATE_NUM_6, LED_WAITTING);
                                set_gate_state(GATE_6, GATE_DISCHARGE);
                            }
                        }
                        if (cJSON_IsNumber(order_time)) {
                            int odt = order_time->valueint;
                            //printf("Gun6 order_time: %d\r\n", odt);
                        }
                        if (cJSON_IsNumber(limit_time)) {
                            int lmt = limit_time->valueint;
                            //printf("Gun6 limit_time: %d\r\n", lmt);
                        }
                    }
                    if(V!=NULL){
                    int value = V->valueint;
                     //respond_to_mqtt(value, 1, 208);
                    }
                    
                     //xQueueSend(publish_queue_handle,"Hardware_respond",portMAX_DELAY);
            //        }
                }
                else if (cmd_type == 102) {
                    const cJSON *action = cJSON_GetObjectItemCaseSensitive(data_new, "action");
                    if (cJSON_IsNumber(action)) {
                        int act = action->valueint;
                        if (act == 1) {
                            ESP_LOGI("COMMAND_102", "Restart device");
                            vTaskDelay(1000 / portTICK_PERIOD_MS);
                            esp_restart();
                        }
                    }
                }
                else if (cmd_type == 103) {
                    printf("===report error===\r\n");
                    printf("->Reset Error state\r\n");
                }
                else if (cmd_type == 104) {
                    const cJSON *lock_state = cJSON_GetObjectItemCaseSensitive(data_new, "lock_state");
                    if (cJSON_IsNumber(lock_state)) {
                        int lockst = lock_state->valueint;
                        if (lockst == 0)
                            ESP_LOGI("COMMAND_104", "Unlocked device");
                        else
                            ESP_LOGI("COMMAND_104", "Locked device");
                    }
                }
                else if (cmd_type == 400) {
                    printf("===OTA update firmware===\r\n");
                    const cJSON *ver = cJSON_GetObjectItemCaseSensitive(data_new, "ver");
                    const cJSON *link1 = cJSON_GetObjectItemCaseSensitive(data_new, "link1");
                    const cJSON *link2 = cJSON_GetObjectItemCaseSensitive(data_new, "link2");
                    if (cJSON_IsString(ver)) {
                        char *v = ver->valuestring;
                        printf("->updating %s version\r\n", v);
                    }
                    if (cJSON_IsString(link2)) {
                        char *lnk2 = link2->valuestring;
                        printf("->updating from %s\r\n", lnk2);
                        printf("Starting download........\r\n");
                       // publish_version(HW_VERSION,FW_VERSION,2);
                        //vTaskDelay(3000/portTICK_PERIOD_MS);
                        //read_enable=false;
                        //pzem_read_enable = false;
                        //vTaskDelay(2000/portTICK_PERIOD_MS);
                       // if (pzem_ds18b20_task != NULL) {
                       //     vTaskDelete(pzem_ds18b20_task);
                    //         pzem_ds18b20_task=NULL;
                    //     }
                    //     if(read_sim_uart_task !=NULL){
                    //         vTaskDelete(read_sim_uart_task);
                    //         read_sim_uart_task=NULL;
                    //     }
                    //    // uart_driver_delete(UART_NUM_2);
                    //     send_at_get_respond("AT\r\n",1000); 
                    //     send_at_get_respond("AT+QMTCLOSE=1\r\n",5000);
                    //     vTaskDelay(1000/portTICK_PERIOD_MS);
                    //     OTA_from_link_v2(lnk2,300000); //https://raw.githubusercontent.com/Minhhauier/IOT_project/main/blink.bin
                        esp_restart();
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
}

// char *make_json_errorcode(int error) {
//     cJSON *data = cJSON_CreateObject();
//     cJSON_AddNumberToObject(data, "GPS", error);
//     char *json = cJSON_PrintUnformatted(data);
//     cJSON_Delete(data);
//     return json;
// }