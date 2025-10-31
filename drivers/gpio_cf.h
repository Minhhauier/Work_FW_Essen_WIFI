#ifndef CONFIG_GPIO
#define CONFIG_GPIO

#define ZERO_DETECT             GPIO_NUM_34
#define LED_DECTEC_MQTT         GPIO_NUM_2

#include <stdint.h>
extern uint8_t g_gate_state;
extern int control_signal;
void config_gpio_detect_zero(void);
void config_gpio_led();

#endif