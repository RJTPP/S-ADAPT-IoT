#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "stm32l4xx_hal.h"

void ultrasonic_init(TIM_HandleTypeDef *tim, uint32_t channel);
uint32_t ultrasonic_read_echo_us(uint32_t timeout_us);
uint32_t ultrasonic_read_distance_cm(uint32_t timeout_us, uint32_t error_value_cm);

#endif /* ULTRASONIC_H */
