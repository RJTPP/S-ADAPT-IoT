#ifndef FILTER_UTILS_H
#define FILTER_UTILS_H

#include "stm32l4xx_hal.h"

#define FILTER_MA_U16_MAX_WINDOW 16U

typedef struct
{
    uint16_t buffer[FILTER_MA_U16_MAX_WINDOW];
    uint32_t sum;
    uint8_t window_size;
    uint8_t count;
    uint8_t index;
} filter_moving_average_u16_t;

void filter_moving_average_u16_init(filter_moving_average_u16_t *f, uint8_t window_size);
uint16_t filter_moving_average_u16_push(filter_moving_average_u16_t *f, uint16_t sample);
uint16_t filter_moving_average_u16_get(const filter_moving_average_u16_t *f);
uint8_t filter_moving_average_u16_is_ready(const filter_moving_average_u16_t *f);

typedef struct
{
    uint32_t samples[3];
    uint8_t count;
    uint8_t index;
} filter_median3_u32_t;

void filter_median3_u32_init(filter_median3_u32_t *f);
void filter_median3_u32_push(filter_median3_u32_t *f, uint32_t sample);
uint32_t filter_median3_u32_get(const filter_median3_u32_t *f);
uint8_t filter_median3_u32_is_ready(const filter_median3_u32_t *f);

#endif /* FILTER_UTILS_H */
