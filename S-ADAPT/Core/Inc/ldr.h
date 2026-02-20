#ifndef LDR_H
#define LDR_H

#include "stm32l4xx_hal.h"

typedef enum
{
    LDR_STATUS_OK = 0,
    LDR_STATUS_NOT_INIT,
    LDR_STATUS_NULL_PTR,
    LDR_STATUS_START_ERROR,
    LDR_STATUS_POLL_ERROR,
    LDR_STATUS_TIMEOUT,
    LDR_STATUS_STOP_ERROR
} ldr_status_t;

void ldr_init(ADC_HandleTypeDef *hadc);
ldr_status_t ldr_read_raw(uint16_t *out_raw);
const char *ldr_status_to_string(ldr_status_t status);

#endif /* LDR_H */
