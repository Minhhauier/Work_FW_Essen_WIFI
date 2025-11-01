#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"


#include "pzem.h"
#include "config_parameter.h"
#include "uart.h"
#include "mqtt_wifi.h"
#include "control_relay.h"
#include "control_led.h"

#define UART_PZEM_NUM          UART_NUM_2
#define TX_PZEM                 27
#define RX_PZEM                 36

static const char *TAG = "PZEM_UART";
static const char *TAG_temp = "DS18B20";
bool pzem_read_enable=true;
// Tính CRC16 Modbus (chuẩn)
uint16_t modbus_crc16(uint8_t *buf, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t pos = 0; pos < len; pos++) {
        crc ^= buf[pos];
        for (uint8_t i = 0; i < 8; i++) {
            if ((crc & 0x0001) != 0)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

PzemData_t pzem_read_and_feedback(uint8_t pzem_addr) {
    PzemData_t data = {0}; // khởi tạo giá trị 0

    // ===== 1. Tạo frame request =====
    uint8_t frame[8];
    frame[0] = pzem_addr;   // địa chỉ slave
    frame[1] = 0x04;        // Function code
    frame[2] = 0x00;
    frame[3] = 0x00;        // Start address
    frame[4] = 0x00;
    frame[5] = 0x0A;        // Read 10 registers

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;        
    frame[7] = (crc >> 8) & 0xFF; 

    uart_write_bytes(UART_PZEM_NUM, (const char *)frame, 8);

    // ===== 2. Đọc phản hồi =====
    uint8_t rx_buf[128];
    int len = uart_read_bytes(UART_PZEM_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(1000));

    if (len < 23) {
        ESP_LOGW("PZEM", "Too few bytes: %d (addr=0x%02X)", len, pzem_addr);
        return data;
    }

    // ===== 3. Kiểm tra địa chỉ =====
    if (rx_buf[0] != pzem_addr) {
        ESP_LOGE("PZEM", "Addr mismatch! Expect:0x%02X Got:0x%02X", pzem_addr, rx_buf[0]);
        return data;
    }
    else {
        ESP_LOGI("PZEM", "Addr match: 0x%02X", rx_buf[0]);
        //return data;
    }

    // ===== 4. Kiểm tra CRC =====
    uint16_t recv_crc = (rx_buf[len - 1] << 8) | rx_buf[len - 2];
    uint16_t calc_crc = modbus_crc16(rx_buf, len - 2);
    if (calc_crc != recv_crc) {
        ESP_LOGE("PZEM", "CRC mismatch! Calc=0x%04X Recv=0x%04X", calc_crc, recv_crc);
        return data;
    }

    // ===== 5. Parse dữ liệu =====
    uint16_t voltage_raw = (rx_buf[3] << 8) | rx_buf[4];
    uint16_t current_raw = (rx_buf[5] << 8) | rx_buf[6];
    uint32_t power_raw   = ((uint32_t)rx_buf[7] << 24) | ((uint32_t)rx_buf[8] << 16) |
                           ((uint32_t)rx_buf[9] << 8) | rx_buf[10];
    uint32_t energy_raw  = ((uint32_t)rx_buf[11] << 24) | ((uint32_t)rx_buf[12] << 16) |
                           ((uint32_t)rx_buf[13] << 8) | rx_buf[14];
    uint16_t freq_raw    = (rx_buf[17] << 8) | rx_buf[18];
    uint16_t pf_raw      = (rx_buf[19] << 8) | rx_buf[20];
    uint16_t alarm_raw   = (rx_buf[21] << 8) | rx_buf[22];

    // Gán vào struct
    data.addr    = pzem_addr;
    data.voltage = voltage_raw / 10.0f;
    data.current = current_raw / 1000.0f;
    data.power   = power_raw / 10.0f;
    data.energy  = energy_raw / 1000.0f;
    data.freq    = freq_raw / 10.0f;
    data.pf      = pf_raw / 100.0f;
    data.alarm   = alarm_raw;

    // ===== 6. In log =====
    // printf("\n===== PZEM-004T V4 Data (Addr 0x%02X) =====\n", data.addr);
    // printf("Voltage : %.1f V\n", data.voltage);
    // printf("Current : %.3f A\n", data.current);
    // printf("Power   : %.1f W\n", data.power);
    // printf("Energy  : %.3f kWh\n", data.energy);
    // printf("Freq    : %.1f Hz\n", data.freq);
    // printf("PF      : %.2f\n", data.pf);
    // printf("Alarm   : 0x%04X\n", data.alarm);
    // printf("============================================\n");

    return data;
}

void reset_Energy_Pzem(uint8_t pzem_addr) {
    uint8_t frame[4];

    frame[0] = pzem_addr;   // Địa chỉ PZEM
    frame[1] = 0x42;        // Lệnh reset energy
    // CRC sẽ thêm ở cuối

    uint16_t crc = modbus_crc16(frame, 2);
    frame[2] = crc & 0xFF;         // CRC Low byte
    frame[3] = (crc >> 8) & 0xFF;  // CRC High byte

    // In frame truyền
    ESP_LOGI(TAG, "Sending RESET ENERGY command to PZEM address: 0x%02X", pzem_addr);
    printf("TX: ");
    for (int i = 0; i < 4; i++) printf("%02X ", frame[i]);
    printf("\n");

    uart_write_bytes(UART_PZEM_NUM, (const char *)frame, 4);

    // Nhận phản hồi
    uint8_t rx_buffer[32];
    int len = uart_read_bytes(UART_PZEM_NUM, rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(1000));

    if (len > 0) {
        ESP_LOGI(TAG, "Received %d bytes", len);
        printf("RX: ");
        for (int i = 0; i < len; i++) printf("%02X ", rx_buffer[i]);
        printf("\n");
    } else {
        ESP_LOGW(TAG, "Không nhận được phản hồi từ PZEM.");
    }
}

void Calibrate_Pzem(){
    uint8_t frame1_calibrate_Pzem1[6]= {0xF8 , 0x41 , 0x37 , 0x21};
    uint16_t crc = modbus_crc16(frame1_calibrate_Pzem1, 4);
    frame1_calibrate_Pzem1[4] = crc & 0xFF;         // CRC Low
    frame1_calibrate_Pzem1[5] = (crc >> 8) & 0xFF;  // CRC High

        // In ra frame gửi
    ESP_LOGI(TAG, "Sending request:");
    printf("TX: ");
    for (int i = 0; i < 6; i++) printf("%02X ", frame1_calibrate_Pzem1[i]);
    printf("\n");
    uart_write_bytes(UART_PZEM_NUM, (const char *)frame1_calibrate_Pzem1, 6);
     
    vTaskDelay(pdMS_TO_TICKS(3000));  // đợi phản hồi

    uint8_t rx_buffer_Calib_Pzem[128];

     int len = uart_read_bytes(UART_PZEM_NUM, rx_buffer_Calib_Pzem, sizeof(rx_buffer_Calib_Pzem), pdMS_TO_TICKS(1000));

        if (len > 0) {
            ESP_LOGI(TAG, "Received %d bytes", len);
            printf("RX: ");
            for (int i = 0; i < len; i++) printf("%02X ", rx_buffer_Calib_Pzem[i]);
            printf("\n");
        } else {
            ESP_LOGW(TAG, "Không nhận được dữ liệu từ PZEM.");
        }
}
void pzem_set_address(uint8_t old_addr, uint8_t new_addr) {
    uint8_t frame[8];
    frame[0] = old_addr;       // Địa chỉ cũ
    frame[1] = 0x06;           // Function code: Write single register
    frame[2] = 0x00;           // Address MSB
    frame[3] = 0x02;           // Address LSB → Register 0x0002
    frame[4] = 0x00;           // Value MSB
    frame[5] = new_addr;       // Value LSB (địa chỉ mới)

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;     // CRC Low
    frame[7] = (crc >> 8) & 0xFF; // CRC High

    // Gửi frame
    uart_write_bytes(UART_PZEM_NUM, (const char *)frame, 8);
    printf("Set addr frame: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", frame[i]);
    }
    printf("\n");

    uint8_t rx_buffer_set_add[128];

     int len = uart_read_bytes(UART_PZEM_NUM, rx_buffer_set_add, sizeof(rx_buffer_set_add), pdMS_TO_TICKS(1000));

        if (len > 0) {
            ESP_LOGI(TAG, "Received %d bytes", len);
            printf("RX: ");
            for (int i = 0; i < len; i++) printf("%02X ", rx_buffer_set_add[i]);
            printf("\n");
        } else {
            ESP_LOGW(TAG, "Không nhận được dữ liệu từ PZEM.");
        }

}



void pzem_task(void *pvParameters) {

    configure_uart_dynamic_Pzem(UART_PZEM_NUM, 9600, TX_PZEM, RX_PZEM);

    gpio_set_pull_mode(DS18B20_GPIO, GPIO_PULLUP_ONLY); // DS18B20 cần điện trở kéo lên 4.7k
    TickType_t sim_start_tick = xTaskGetTickCount();
    TickType_t temp_start_tick = xTaskGetTickCount();
    float power[6]={0,0,0,0,0,0};
    int vol[6]={0,0,0,0,0,0};
    int count[6]={0,0,0,0,0,0};
    while (1) {
        if(pzem_read_enable)
        {
        int detect_power=0;
        for (uint8_t addr = 0x01; addr <= 0x04; addr++) {
            PzemData_t data = pzem_read_and_feedback(addr);
            vTaskDelay(pdMS_TO_TICKS(500));  
            // delay ngắn giữa các PZEM để UART không bị dính dữ liệu
            if(data.power>=POWER_MIN) power[addr-1] = data.power;
            else power[addr-1]=0;
            if(power[addr-1]==0){
                if(get_gate_state(addr)==GATE_CHARGE) count[addr-1]++;
                else count[addr-1]=0;
                if(count[addr-1]>=20){
                 set_gate_state(addr,GATE_DISCHARGE);
                 set_group_led(&charge_led,COLOR_BLACK,addr);
                 count[addr-1]=0;
                }
            }
            else count[addr-1]=0;
            vol[addr-1] = data.voltage;
            vTaskDelay(pdMS_TO_TICKS(500)); 
            if(data.power>=POWER_MIN) detect_power=1;
        }
        if(detect_power){
            if(xTaskGetTickCount() - sim_start_tick >= pdMS_TO_TICKS(10000)){
                sim_start_tick = xTaskGetTickCount();
                mqtt_publish_data_power(power,vol);
                vTaskDelay(pdMS_TO_TICKS(100)); 
            }
        }
        if(xTaskGetTickCount() - temp_start_tick >= pdMS_TO_TICKS(150000)){
            TempData_t t = ds18b20_read_temp_struct();
            if (t.valid) {
                ESP_LOGI("DS18B20", "Temp: %.2f C (time=%llu us)", t.value, t.timestamp);
                temp_start_tick = xTaskGetTickCount();
                mqtt_publish_temp(t.value);
                
            } else {
                ESP_LOGW("DS18B20", "Read error");
            }
        }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 2s đọc 1 lần
    }
}


// void configure_uart_dynamic_Pzem(uart_port_t uart_num, int baudrate, int tx_pin, int rx_pin) {
//     uart_config_t uart_config = {
//         .baud_rate = baudrate,
//         .data_bits = UART_DATA_8_BITS,
//         .parity    = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_APB,  // Dùng APB Clock
//     };

//     ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
//     ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
//     ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));

//     ESP_LOGI("UART", "UART%d configured: baud=%d, TX=%d, RX=%d", uart_num, baudrate, tx_pin, rx_pin);
// }

// Đọc nhiệt độ từ DS18B20
// ==================== OneWire Low-Level ====================
static void onewire_write_bit(int bit)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    if (bit) {
        esp_rom_delay_us(6);
        gpio_set_level(DS18B20_GPIO, 1);
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        gpio_set_level(DS18B20_GPIO, 1);
        esp_rom_delay_us(10);
    }
}

static int onewire_read_bit(void)
{
    int r;
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(6);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(9);
    r = gpio_get_level(DS18B20_GPIO);
    esp_rom_delay_us(55);
    return r;
}

static void onewire_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t onewire_read_byte(void)
{
    uint8_t r = 0;
    for (int i = 0; i < 8; i++) {
        r >>= 1;
        if (onewire_read_bit()) {
            r |= 0x80;
        }
    }
    return r;
}

static int onewire_reset(void)
{
    int r;
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    esp_rom_delay_us(480);
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);
    r = gpio_get_level(DS18B20_GPIO);
    esp_rom_delay_us(410);
    return !r; // 0=No presence, 1=Presence detected
}

// ==================== DS18B20 functions ====================

// Hàm cũ: chỉ trả về float
float ds18b20_read_temp(void)
{
    if (!onewire_reset()) {
        ESP_LOGE(TAG_temp, "No DS18B20 detected");
        return -1000.0;
    }
    onewire_write_byte(0xCC); // Skip ROM
    onewire_write_byte(0x44); // Convert T
    vTaskDelay(pdMS_TO_TICKS(750)); // Max conversion time

    if (!onewire_reset()) {
        ESP_LOGE(TAG_temp, "No DS18B20 detected after convert");
        return -1000.0;
    }
    onewire_write_byte(0xCC); // Skip ROM
    onewire_write_byte(0xBE); // Read Scratchpad

    uint8_t temp_lsb = onewire_read_byte();
    uint8_t temp_msb = onewire_read_byte();
    int16_t raw = (temp_msb << 8) | temp_lsb;

    return (float)raw / 16.0; // °C
}

// ==================== Struct để đọc nhiệt độ ====================
TempData_t ds18b20_read_temp_struct(void)
{
    TempData_t result = {
        .value = -1000.0,
        .valid = 0,
        .timestamp = esp_timer_get_time()
    };

    float temp = ds18b20_read_temp();
    if (temp > -200 && temp < 125) {
        result.value = temp;
        result.valid = 1;
        result.timestamp = esp_timer_get_time();
    }
    return result;
}
