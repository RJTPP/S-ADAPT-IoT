#ifndef ENCODER_INPUT_H
#define ENCODER_INPUT_H

#include "stm32l4xx_hal.h"

typedef enum
{
    ENCODER_EVENT_CW = 0,
    ENCODER_EVENT_CCW,
    ENCODER_EVENT_SW_PRESSED,
    ENCODER_EVENT_SW_RELEASED
} encoder_event_type_t;

typedef struct
{
    encoder_event_type_t type;
    uint32_t timestamp_ms;
    uint8_t sw_level;
} encoder_event_t;

void encoder_input_init(void);
void encoder_input_tick(uint32_t now_ms);
void encoder_input_on_clk_edge_isr(void);
uint8_t encoder_input_pop_event(encoder_event_t *out_event);

#endif /* ENCODER_INPUT_H */
