#ifndef APP_H
#define APP_H

#include "stm32l4xx_hal.h"

uint8_t app_init(TIM_HandleTypeDef *echo_tim, uint32_t echo_channel);
void app_step(void);

#endif /* APP_H */
