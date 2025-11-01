#ifndef PZEM_H
#define PZEM_H

#include <stdint.h>
#include "driver/uart.h"
#include "driver/gpio.h"



#define DS18B20_GPIO 5


typedef struct {
    float value;        // Giá trị nhiệt độ (°C)
    int valid;          // =1 nếu đọc OK, =0 nếu lỗi
    uint64_t timestamp; // Thời gian đọc (micro giây)
} TempData_t;


typedef struct {
    uint8_t  addr;       // Địa chỉ slave PZEM
    float    voltage;    // V
    float    current;    // A
    float    power;      // W
    float    energy;     // kWh
    float    freq;       // Hz
    float    pf;         // Power factor
    uint16_t alarm;      // Alarm flag
} PzemData_t;

PzemData_t pzem_read_and_feedback(uint8_t pzem_addr);

void pzem_task(void *pvParameters);


TempData_t ds18b20_read_temp_struct(void);

#endif // PZEM_H
