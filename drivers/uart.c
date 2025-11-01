#include <driver/uart.h>
#include <driver/gpio.h>
#include <config_parameter.h>
#include <esp_log.h>

void configure_uart_dynamic_Pzem(uart_port_t uart_num, int baudrate, int tx_pin, int rx_pin) {
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,  // Dùng APB Clock
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE_PZEM * 2, 0, 0, NULL, 0));

    ESP_LOGI("UART_PZEM", "UART%d configured: baud=%d, TX=%d, RX=%d", uart_num, baudrate, tx_pin, rx_pin);
}