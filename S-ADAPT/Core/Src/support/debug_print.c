#include "support/debug_print.h"

#include <stdarg.h>
#include <stdio.h>

static UART_HandleTypeDef *s_debug_uart = NULL;
static debug_print_level_t s_debug_level = DEBUG_PRINT_INFO;

static void debug_vprint(debug_print_level_t level, uint8_t with_newline, const char *fmt, va_list args)
{
    char buffer[128];
    int len;

    if (s_debug_uart == NULL || fmt == NULL) {
        return;
    }
    if ((uint32_t)level > (uint32_t)s_debug_level) {
        return;
    }

    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (len < 0) {
        return;
    }

    if (with_newline != 0U) {
        if ((size_t)len > (sizeof(buffer) - 3U)) {
            len = (int)(sizeof(buffer) - 3U);
        }
        buffer[len++] = '\r';
        buffer[len++] = '\n';
        buffer[len] = '\0';
    } else {
        if ((size_t)len > (sizeof(buffer) - 1U)) {
            len = (int)(sizeof(buffer) - 1U);
        }
    }

    HAL_UART_Transmit(s_debug_uart, (uint8_t *)buffer, (uint16_t)len, 100U);
}

void debug_print_init(UART_HandleTypeDef *huart)
{
    s_debug_uart = huart;
}

void debug_print_set_level(debug_print_level_t level)
{
    s_debug_level = level;
}

debug_print_level_t debug_print_get_level(void)
{
    return s_debug_level;
}

void debug_log(debug_print_level_t level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    debug_vprint(level, 0U, fmt, args);
    va_end(args);
}

void debug_logln(debug_print_level_t level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    debug_vprint(level, 1U, fmt, args);
    va_end(args);
}

void debug_print(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    debug_vprint(DEBUG_PRINT_INFO, 0U, fmt, args);
    va_end(args);
}

void debug_println(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    debug_vprint(DEBUG_PRINT_INFO, 1U, fmt, args);
    va_end(args);
}
