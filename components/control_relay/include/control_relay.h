#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

typedef enum {
    GATE_NUM_1 = 1,
    GATE_NUM_2 = 2,
    GATE_NUM_3 = 3,
    GATE_NUM_4 = 4,
    GATE_NUM_5 = 5,
    GATE_NUM_6 = 6 
} gate_num_t;

typedef enum {
    GATE_DISCHARGE = 0,
    GATE_CHARGE = 1
} gate_state_t;

void power_read_init();
void relay_init();
void is_relay_init();
void set_gate_state(int gate_num, gate_state_t state);
gate_state_t get_gate_state(int gate_num);

void is_relay_init();
void relay_set(uint8_t value);
void off_all_gate();
#endif // RELAY_CONTROL_H