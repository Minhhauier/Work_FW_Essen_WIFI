#ifndef CONFIG_GPIO
#define CONFIG_GPIO

#include <driver/gpio.h>
#define ZERO_DETECT             GPIO_NUM_34
#define LED_DECTEC_MQTT         GPIO_NUM_4
#define GPIO_WIFI_CONFIG        GPIO_NUM_0

#include <stdint.h>
extern uint8_t g_gate_state;
extern int control_signal;
void config_gpio_detect_zero(void);
void config_gpio_led();
void config_gpio_wifi_menu_config(void);
void stop_action_timer_callback(void *arg);
void start_stop_timer(void);
void stop_my_timer(void);
void detect_wifi_task();
void timer_get_ping_off_gate();
void start_timer_off_all_gate();
void stop_timer_off_gate();
#endif