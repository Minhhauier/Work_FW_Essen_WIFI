#ifndef CONFIG_PARAMETER_H
#define CONFIG_PARAMETER_H

#include <driver/gpio.h>

// DEVICE_PROD for production device, DEVICE_DEV for development device
#define DEVICE_DEV 1

#ifdef DEVICE_DEV
    #define DEVICE_NAME "EVsafe"
    #define SUB         "SU4G"
    #define PUB         "UP4G"
#endif    
#ifdef DEVICE_PROD
    #define DEVICE_NAME "PEVsafe"
    #define SUB         "PSU4G"
    #define PUB         "PUP4G"
#endif

// Remember to change versions before building firmware and up git
#define HW_VERSION      "1.0.3"
#define FW_VERSION      "1.0.5"

//config parameter mqtt
#define BUF_SIZE_MQTT 1024 

//config parameter for relay
#define GATE_1                  GPIO_NUM_23
#define GATE_2                  GPIO_NUM_22
#define GATE_3                  GPIO_NUM_19
#define GATE_4                  GPIO_NUM_18
#define GATE_5                  GPIO_NUM_26
#define GATE_6                  GPIO_NUM_4

#define RELAY_SLCH              GPIO_NUM_25
#define RELAY_SLCK              GPIO_NUM_33
#define RELAY_SDAT              GPIO_NUM_32

//config parameter for led
#define CHARGE_LED_GPIO          15
#define CHARGE_LED_NUMBER        18
#define CHARGE_LED_LEN           3

#define CONNECTION_LED_GPIO      1
#define CONNECTION_LED_NUMBER    1
#define CONNECTION_LED_LEN       1

#endif