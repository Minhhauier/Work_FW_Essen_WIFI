#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

#include "control_led.h"
#include "control_relay.h"
#include "config_parameter.h"

led_config_t charge_led = {
    .handle = NULL,
    .led_gpio = CHARGE_LED_GPIO,
    .led_count = CHARGE_LED_NUMBER,
    .group_size = CHARGE_LED_LEN,
};
led_task_param_t *params;
void start_charge_led() {
    led_strip_init(&charge_led);
    set_group_led(&charge_led, COLOR_RED, 3);
    printf("Start charge LED\n");
}

bool check_led_init = false;
bool active = false;
int gate;


void charging_led_by_status(int gate_num, int status) {
    if (!check_led_init) {
        led_strip_init(&charge_led);
        check_led_init = true;
       // is_relay_init();
        //relay_set(0b00000000);
        
        printf("led ready\r\n");
    }
    switch (status) {
        case LED_OFF:
            set_group_led(&charge_led, COLOR_BLACK, gate_num);
            active = false;
            break;
        case LED_READY:
            set_group_led(&charge_led, COLOR_BLACK, gate_num);
            active = false;
            break;
        case LED_CHARGING:
            set_group_led(&charge_led, COLOR_GREEN, gate_num);
            active = false;
            break;
        case LED_ERROR:
            set_group_led(&charge_led, COLOR_RED, gate_num);
            active = false;
            break;
        case LED_WAITTING:
            set_group_led(&charge_led, COLOR_ORANGE, gate_num);
            break;
        default:
            printf("Invalid LED status\n");
            active = false;
            break;
    }
}

void all_led_by_status(int status) {
    if (!check_led_init) {
        led_strip_init(&charge_led);
        off_all_gate();
        check_led_init = true;
    }
    switch (status) {
        case LED_OFF:
            led_strip_clear_all(&charge_led);
            break;
        case LED_READY:
            set_all_led(&charge_led, COLOR_BLUE);
            break;
        case LED_CHARGING:
            set_all_led(&charge_led, COLOR_GREEN);
            break;
        case LED_WARNING:
            set_all_led(&charge_led, COLOR_ORANGE);
            break;
        case LED_ERROR:
            set_all_led(&charge_led, COLOR_RED);
            break;
        default:
            printf("Invalid LED status\n");
            break;
    }
   // set_group_led(&charge_led,COLOR_RED,7);
}


void led_blink(led_task_param_t *params){
    while (1)
    {
        if(active){
            set_group_led(params->led_strip, COLOR_ORANGE, params->index);
            vTaskDelay(pdMS_TO_TICKS(1000));
            clear_group_led(params->led_strip,params->index);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        else{
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    
}



