#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "stm32l4xx_hal.h"

typedef enum
{
    ULTRASONIC_STATUS_OK = 0,
    ULTRASONIC_STATUS_NOT_INIT,
    ULTRASONIC_STATUS_INVALID_CHANNEL,
    ULTRASONIC_STATUS_TIMEOUT_RISING,
    ULTRASONIC_STATUS_TIMEOUT_FALLING,
    ULTRASONIC_STATUS_OVERCAPTURE_RISING,
    ULTRASONIC_STATUS_OVERCAPTURE_FALLING
} ultrasonic_status_t;

void ultrasonic_init(TIM_HandleTypeDef *tim, uint32_t channel);
uint32_t ultrasonic_read_echo_us(uint32_t timeout_us);
uint32_t ultrasonic_read_distance_cm(uint32_t timeout_us, uint32_t error_value_cm);
ultrasonic_status_t ultrasonic_get_last_status(void);
const char *ultrasonic_status_to_string(ultrasonic_status_t status);

#endif /* ULTRASONIC_H */
