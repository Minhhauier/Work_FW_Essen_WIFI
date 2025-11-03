#include <stdbool.h>
#include <stdint.h>

#include "control_relay.h"
#include "config_parameter.h"
#include "gpio_cf.h"

//config
void power_read_init() {
    uint64_t pins_mask = (1ULL << GATE_1) |
                         (1ULL << GATE_2) |
                         (1ULL << GATE_3) |
                         (1ULL << GATE_4) |
                         (1ULL << GATE_5) |
                         (1ULL << GATE_6);

    gpio_config_t io_conf = {
        .pin_bit_mask = pins_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void relay_init() {
    uint64_t pins_mask = (1ULL << RELAY_SDAT) |
                         (1ULL << RELAY_SLCH) |
                         (1ULL << RELAY_SLCK);

    gpio_config_t io_conf = {
        .pin_bit_mask = pins_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
    //off_all_gate();
}
void relay_set(uint8_t value){
    int i;
    gpio_set_level(RELAY_SLCH, 0);
    for (i = 7; i >= 0; i--){
        gpio_set_level(RELAY_SDAT, (value >> i) & 1);
        gpio_set_level(RELAY_SLCK, 1);
        gpio_set_level(RELAY_SLCK, 0);
    }
    gpio_set_level(RELAY_SLCH, 1);
}

//application
uint8_t g_gate_state = 0b00000000;

bool relay_is_inited = false;

void is_relay_init() {
    if (!relay_is_inited) {
        relay_init();
        relay_set(g_gate_state);
        relay_is_inited = true;
    }
}

void set_gate_state(int gate_num, gate_state_t state) {
    is_relay_init();
    if (gate_num < 1 || gate_num > 6) {
        return;  // Invalid gate number
    }
    switch (state) {
        case GATE_CHARGE:
            g_gate_state |= (0b00000001 << (gate_num - 1));
            break;
        case GATE_DISCHARGE:
            g_gate_state &= ~(0b00000001 << (gate_num - 1));
            break;
        default:
            return;  // Invalid state 
    }
    if(control_signal!=1) {       
        relay_set(g_gate_state);
        printf("Set relay\r\n");
    }
    //relay_set(g_gate_state);
}

gate_state_t get_gate_state(int gate_num) {
    if (gate_num < 1 || gate_num > 6) {
        return -1;  // Invalid gate number or null pointer
    }
    if (g_gate_state & (0b00000001 << (gate_num - 1))) {
        return GATE_CHARGE;
    }
    return GATE_DISCHARGE;
}
void off_all_gate(){
    set_gate_state(GATE_1, GATE_DISCHARGE);
    set_gate_state(GATE_2, GATE_DISCHARGE);
    set_gate_state(GATE_3, GATE_DISCHARGE);
    set_gate_state(GATE_4, GATE_DISCHARGE);
    set_gate_state(GATE_5, GATE_DISCHARGE);
    set_gate_state(GATE_6, GATE_DISCHARGE);
}