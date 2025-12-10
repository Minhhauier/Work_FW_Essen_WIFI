#include <driver/gpio.h>

#include "gpio_cf.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "control_relay.h"

#include "setup_wifi.h"
#include "esp_wifi.h"
#include "control_led.h"

#define DELAY_US 15000  // 15ms

static esp_timer_handle_t delay_timer; 
static esp_timer_handle_t stop_timer = NULL;
static esp_timer_handle_t get_ping_timer = NULL; 
static esp_timer_handle_t timer_off_gate = NULL;

// callback sau 15ms 
static void delay_timer_callback(void* arg) {
    if(control_signal==1)
    {
    printf("done\n");
    control_signal=0;
    relay_set(g_gate_state);
    }
}
// phát hiện sườn âm
static void IRAM_ATTR zcd_isr_handler(void* arg) {
    // Bắt đầu đếm 15ms
    esp_timer_start_once(delay_timer, DELAY_US);
}

void config_gpio_detect_zero(void){
    gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << ZERO_DETECT),      // Select GPIO 
    .mode = GPIO_MODE_INPUT,            // Set as input
    .pull_up_en = GPIO_PULLUP_DISABLE,  // Disable pull-up
    .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Disable pull-down
    .intr_type = GPIO_INTR_NEGEDGE             // Disable interrupts
};
gpio_config(&io_conf);
// tạo timer delay 5ms 
const esp_timer_create_args_t timer_args = {
    .callback = &delay_timer_callback,
    .name = "delay_timer"
};
esp_timer_create(&timer_args, &delay_timer);


gpio_install_isr_service(0);
gpio_isr_handler_add(ZERO_DETECT, zcd_isr_handler, NULL);
printf("Zero-cross detection started!\n");

}
void config_gpio_led(){
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_DECTEC_MQTT),      // Select GPIO 2
        .mode = GPIO_MODE_OUTPUT,            // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,  // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE             // Disable interrupts
    };

    gpio_config(&io_conf);
}

void config_gpio_wifi_menu_config(void){
    gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << GPIO_WIFI_CONFIG),      // Select GPIO 
    .mode = GPIO_MODE_INPUT,            // Set as input
    .pull_up_en = GPIO_PULLUP_ENABLE,  // Disable pull-up
    .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Disable pull-down
    .intr_type = GPIO_INTR_DISABLE             
};
gpio_config(&io_conf);

// gpio_install_isr_service(0);
}

void stop_action_timer_callback(void *arg)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI("WIFI", "AP interface disabled - Device now in pure Station mode");
    wifi_state=1;
}

void start_stop_timer(void)
{
    if(stop_timer!=NULL){
        esp_timer_stop(stop_timer);
        esp_timer_delete(stop_timer);
        stop_timer = NULL;
    }
    const esp_timer_create_args_t stop_timer_args = {
        .callback = &stop_action_timer_callback,
        .name = "stop_action_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&stop_timer_args, &stop_timer));

    // 3 phút = 180 giây = 180 * 1,000,000 micro giây
    ESP_ERROR_CHECK(esp_timer_start_once(stop_timer, 180000000ULL));
    ESP_LOGI("TIMER", "Stop timer started (3 minutes)");
}
void stop_my_timer(void)
{
    if (stop_timer != NULL) {
        esp_timer_stop(stop_timer);
        esp_timer_delete(stop_timer);
        stop_timer = NULL;
        ESP_LOGI("TIMER", "Stop timer manually stopped");
    }
}
void off_gate_call_back(){
    control_signal=1;
    all_led_by_status(0);
    off_all_gate();
    printf("Don't get any ping data, off all gates\r\n");
}

void timer_get_ping_off_gate(){
    if(get_ping_timer!=NULL){
        esp_timer_stop(get_ping_timer);
        esp_timer_delete(get_ping_timer);
        get_ping_timer = NULL;
    }
    const esp_timer_create_args_t get_ping_timer_args = {
        .callback = &off_gate_call_back,
        .name = "get_ping_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&get_ping_timer_args, &get_ping_timer));

    ESP_ERROR_CHECK(esp_timer_start_once(get_ping_timer, 300000000ULL));// call back after 5 minutes 
    ESP_LOGI("TIMER", "Get ping");
}
void detect_wifi_task(){
    bool config_mode = false;
    config_gpio_wifi_menu_config();
    while (1)
    {
        if(gpio_get_level(GPIO_WIFI_CONFIG)==0)
        {
            while (gpio_get_level(GPIO_WIFI_CONFIG)==0)
            {
                vTaskDelay(500/portTICK_PERIOD_MS);
            }
            wifi_state=2;
            if(config_mode==false)
            {
            reopen_network();
            start_stop_timer();
            config_mode=true;
            }
            else ESP_LOGI("WIFI","Wifi config mode possible");
        }
        if(wifi_state==0){
            gpio_set_level(LED_DECTEC_MQTT,0);
            config_mode=false;
        }
        else if(wifi_state==1){
            gpio_set_level(LED_DECTEC_MQTT,1);
            config_mode=false;
        }
        else{
            gpio_set_level(LED_DECTEC_MQTT,0);
            vTaskDelay(500/portTICK_PERIOD_MS);
            gpio_set_level(LED_DECTEC_MQTT,1);
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    

}
void stop_timer_off_gate(){
    if (timer_off_gate != NULL){
        esp_timer_stop(timer_off_gate);
        esp_timer_delete(timer_off_gate);
        timer_off_gate = NULL;
        ESP_LOGI("TIMER_gate", "Stop timer off all gates");
    }
}

void off_gate_action_call_back(){
    control_signal=1;
    printf("stopped all gates\r\n");
    all_led_by_status(0);
    off_all_gate();
}
void start_timer_off_all_gate(){
    if(timer_off_gate!=NULL){
        // esp_timer_stop(timer_off_gate);
        // esp_timer_delete(timer_off_gate);
        // timer_off_gate=NULL;
        return;
    }
    const esp_timer_create_args_t off_gate_timer_args = {
        .callback = &off_gate_action_call_back,
        .name = "start_timer_to_off_gates"
    };
    ESP_ERROR_CHECK(esp_timer_create(&off_gate_timer_args, &timer_off_gate));

    ESP_ERROR_CHECK(esp_timer_start_once(timer_off_gate, 300000000ULL));// 300 seconds
    //ESP_ERROR_CHECK(esp_timer_start_once(timer_off_gate, 60000000ULL));// 300 seconds
    ESP_LOGI("TIMER_OFF_GATE", "all gate off after 5 minutes");
}