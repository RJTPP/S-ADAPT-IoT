#include "display.h"

#include "fonts.h"
#include "ssd1306.h"

#include <stdio.h>

uint8_t display_init(void)
{
    return SSD1306_Init() ? 1U : 0U;
}

void display_show_boot(void)
{
    SSD1306_Fill(Black);
    SSD1306_SetCursor(10, 10);
    SSD1306_WriteString("Init OK", Font16x24, White);
    SSD1306_UpdateScreen();
    HAL_Delay(1000);
}

void display_show_distance_cm(uint32_t distance_cm)
{
    char dist_str[20];

    (void)snprintf(dist_str, sizeof(dist_str), "Dist: %lu cm", (unsigned long)distance_cm);
    SSD1306_Fill(Black);
    SSD1306_SetCursor(10, 10);
    SSD1306_WriteString(dist_str, Font12x12, White);
    SSD1306_UpdateScreen();
}
