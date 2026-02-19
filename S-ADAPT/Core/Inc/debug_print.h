#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include "stm32l4xx_hal.h"

void debug_print_init(UART_HandleTypeDef *huart);
void debug_print(const char *fmt, ...);
void debug_println(const char *fmt, ...);

#endif /* DEBUG_PRINT_H */
