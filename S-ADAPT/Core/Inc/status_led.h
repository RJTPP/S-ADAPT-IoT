#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "stm32l4xx_hal.h"

/* 0: logical ON drives GPIO_PIN_SET, 1: logical ON drives GPIO_PIN_RESET */
#ifndef STATUS_LED_ACTIVE_LOW
#define STATUS_LED_ACTIVE_LOW 0
#endif

typedef enum
{
    STATUS_LED_STATE_BOOT_SETUP = 0,
    STATUS_LED_STATE_AUTO,
    STATUS_LED_STATE_OFFSET_POSITIVE,
    STATUS_LED_STATE_NO_USER,
    STATUS_LED_STATE_FAULT_FATAL
} status_led_state_t;

void status_led_init(void);
void status_led_set_state(status_led_state_t state);
void status_led_tick(uint32_t now_ms);
void status_led_set_fatal_fault(uint8_t enabled);

/* Compatibility API: distance-based mapping is deprecated and will be removed. */
void status_led_set_for_distance(uint32_t distance_cm);
/* Compatibility shim: no blocking delay, routes to fault/tick behavior. */
void status_led_blink_error(uint32_t period_ms);

#endif /* STATUS_LED_H */
