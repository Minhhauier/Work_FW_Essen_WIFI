#ifndef UART
#define UART
#include <driver/uart.h>
void configure_uart_dynamic_Pzem(uart_port_t uart_num, int baudrate, int tx_pin, int rx_pin);

#endif