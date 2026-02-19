#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "stm32l4xx_hal.h"

void status_led_set_for_distance(uint32_t distance_cm);
void status_led_blink_error(uint32_t period_ms);

#endif /* STATUS_LED_H */
