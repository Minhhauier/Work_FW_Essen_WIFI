#include <driver/gpio.h>

#include "gpio_cf.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "control_relay.h"
#define DELAY_US 15000  // 15ms

static esp_timer_handle_t delay_timer; 

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