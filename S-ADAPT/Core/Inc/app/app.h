#ifndef APP_H
#define APP_H

#include "stm32l4xx_hal.h"

uint8_t app_init(ADC_HandleTypeDef *ldr_adc,
                 TIM_HandleTypeDef *echo_tim,
                 uint32_t echo_channel,
                 TIM_HandleTypeDef *main_led_tim,
                 uint32_t main_led_channel);
void app_step(void);
void app_set_fatal_fault(uint8_t enabled);

#endif /* APP_H */
