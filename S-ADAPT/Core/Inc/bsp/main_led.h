#ifndef MAIN_LED_H
#define MAIN_LED_H

#include "stm32l4xx_hal.h"

typedef enum
{
    MAIN_LED_STATUS_OK = 0,
    MAIN_LED_STATUS_NOT_INIT,
    MAIN_LED_STATUS_NULL_PTR,
    MAIN_LED_STATUS_INVALID_PERCENT,
    MAIN_LED_STATUS_HAL_START_ERROR
} main_led_status_t;

void main_led_init(TIM_HandleTypeDef *htim, uint32_t channel);
main_led_status_t main_led_start(void);
main_led_status_t main_led_set_percent(uint8_t percent);
uint8_t main_led_get_percent(void);
main_led_status_t main_led_set_enabled(uint8_t enabled);
const char *main_led_status_to_string(main_led_status_t status);

#endif /* MAIN_LED_H */
