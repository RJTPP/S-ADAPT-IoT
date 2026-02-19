#include "display.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>

uint8_t display_init(void)
{
    if (HAL_I2C_IsDeviceReady(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 2, 50) != HAL_OK) {
        return 0U;
    }

    ssd1306_Init();
    return 1U;
}

void display_show_boot(void)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(10, 10);
    ssd1306_WriteString((char *)"Init OK", Font_16x24, White);
    ssd1306_UpdateScreen();
    HAL_Delay(1000);
}

void display_show_distance_cm(uint32_t distance_cm)
{
    char dist_str[20];
    int n;

    n = snprintf(dist_str, sizeof(dist_str), "Dist: %lu cm", (unsigned long)distance_cm);
    if (n < 0) {
        return;
    }

    ssd1306_Fill(Black);
    ssd1306_SetCursor(10, 10);
    ssd1306_WriteString(dist_str, Font_7x10, White);
    ssd1306_UpdateScreen();
}
