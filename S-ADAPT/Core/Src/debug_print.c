#include "debug_print.h"

#include <stdarg.h>
#include <stdio.h>

static UART_HandleTypeDef *s_debug_uart = NULL;

void debug_print_init(UART_HandleTypeDef *huart)
{
    s_debug_uart = huart;
}

void debug_print(const char *fmt, ...)
{
    char buffer[128];
    int len;
    va_list args;

    if (s_debug_uart == NULL || fmt == NULL) {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len <= 0) {
        return;
    }

    if ((size_t)len > (sizeof(buffer) - 1U)) {
        len = (int)(sizeof(buffer) - 1U);
    }

    HAL_UART_Transmit(s_debug_uart, (uint8_t *)buffer, (uint16_t)len, 100U);
}

void debug_println(const char *fmt, ...)
{
    char buffer[128];
    int len;
    va_list args;

    if (s_debug_uart == NULL || fmt == NULL) {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) {
        return;
    }

    if ((size_t)len > (sizeof(buffer) - 3U)) {
        len = (int)(sizeof(buffer) - 3U);
    }

    buffer[len++] = '\r';
    buffer[len++] = '\n';
    buffer[len] = '\0';

    HAL_UART_Transmit(s_debug_uart, (uint8_t *)buffer, (uint16_t)len, 100U);
}
