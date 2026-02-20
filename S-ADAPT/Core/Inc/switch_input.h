#ifndef SWITCH_INPUT_H
#define SWITCH_INPUT_H

#include "stm32l4xx_hal.h"

typedef enum
{
    SWITCH_INPUT_BUTTON = 0,
    SWITCH_INPUT_SW2 = 1
} switch_input_id_t;

typedef struct
{
    switch_input_id_t input;
    uint8_t pressed;
    uint8_t level;
} switch_input_event_t;

void switch_input_init(void);
void switch_input_tick(uint32_t now_ms);
uint8_t switch_input_pop_event(switch_input_event_t *out_event);

#endif /* SWITCH_INPUT_H */
