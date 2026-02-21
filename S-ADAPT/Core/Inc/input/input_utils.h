#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include "stm32l4xx_hal.h"

static inline uint32_t input_irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void input_irq_unlock(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

static inline uint8_t input_gpio_level(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static inline uint8_t input_has_elapsed_ms(uint32_t now_ms, uint32_t last_ms, uint32_t period_ms)
{
    return ((uint32_t)(now_ms - last_ms) >= period_ms) ? 1U : 0U;
}

#endif /* INPUT_UTILS_H */
