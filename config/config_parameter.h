#ifndef CONFIG_PARAMETER_H
#define CONFIG_PARAMETER_H

#include <driver/gpio.h>

// DEVICE_PROD for production device, DEVICE_DEV for development device
//#define DEVICE_PROD
#define DEVICE_DEV
#ifdef DEVICE_DEV
    #define DEVICE_NAME "EVsafe"
    #define SUB         "SU"
    #define PUB         "UP"
#endif    
#ifdef DEVICE_PROD
    #define DEVICE_NAME "PEVsafe"
    #define SUB         "PSU"
    #define PUB         "PUP"
#endif

// Remember to change versions before building firmware and up git
#define HW_VERSION      "1.0.3"
#define FW_VERSION      "1.1"

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

// config parameter for pzem
#define UART_PZEM_NUM          UART_NUM_2
#define TX_PZEM                 27
#define RX_PZEM                 36
#define POWER_MIN               5
#define POWER_MAX               3500
#define BUF_SIZE_PZEM           1024
// Config parameter for SIM
#define UART_SIM_NUM           UART_NUM_1
#define TX_SIM                  17
#define RX_SIM                  39
#define UART_BAUD_RATE          115200
#define BUF_SIZE_SIM            1024
//config parameter for led
#define CHARGE_LED_GPIO          15
#define CHARGE_LED_NUMBER        18
#define CHARGE_LED_LEN           3

#define CONNECTION_LED_GPIO      1
#define CONNECTION_LED_NUMBER    1
#define CONNECTION_LED_LEN       1

#endif