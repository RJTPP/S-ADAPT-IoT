#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include "stm32l4xx_hal.h"

typedef enum
{
    DEBUG_PRINT_ERROR = 0,
    DEBUG_PRINT_INFO = 1,
    DEBUG_PRINT_DEBUG = 2
} debug_print_level_t;

void debug_print_init(UART_HandleTypeDef *huart);
void debug_print_set_level(debug_print_level_t level);
debug_print_level_t debug_print_get_level(void);
void debug_log(debug_print_level_t level, const char *fmt, ...);
void debug_logln(debug_print_level_t level, const char *fmt, ...);
void debug_print(const char *fmt, ...);
void debug_println(const char *fmt, ...);

#endif /* DEBUG_PRINT_H */
