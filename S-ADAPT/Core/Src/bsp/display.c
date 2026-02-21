#include "bsp/display.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>

static uint8_t clamp_percent_u8(uint8_t value)
{
    return (value > 100U) ? 100U : value;
}

static int32_t clamp_offset_i32(int32_t value)
{
    if (value < -50) {
        return -50;
    }
    if (value > 50) {
        return 50;
    }
    return value;
}

static void draw_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t percent)
{
    uint8_t fill_width = (uint8_t)(((uint32_t)width * clamp_percent_u8(percent)) / 100U);

    ssd1306_DrawRectangle(x, y, (uint8_t)(x + width), (uint8_t)(y + height), White);
    if (fill_width > 0U) {
        ssd1306_FillRectangle((uint8_t)(x + 1U),
                              (uint8_t)(y + 1U),
                              (uint8_t)(x + fill_width - 1U),
                              (uint8_t)(y + height - 1U),
                              White);
    }
}

static void draw_page_bullets(uint8_t active_page)
{
    const uint8_t y = 60U;
    const uint8_t left_x = 114U;
    const uint8_t right_x = 124U;

    ssd1306_DrawCircle(left_x, y, 2U, White);
    ssd1306_DrawCircle(right_x, y, 2U, White);
    if (active_page == 0U) {
        ssd1306_FillCircle(left_x, y, 2U, White);
    } else {
        ssd1306_FillCircle(right_x, y, 2U, White);
    }
}

static const char *mode_to_text(display_mode_t mode)
{
    switch (mode) {
        case DISPLAY_MODE_OFF:
            return "OFF";
        case DISPLAY_MODE_ON:
            return "ON";
        case DISPLAY_MODE_SLEEP:
            return "SLEEP";
        default:
            return "UNK";
    }
}

static const char *reason_to_text(display_reason_t reason)
{
    switch (reason) {
        case DISPLAY_REASON_AWAY:
            return "AWY";
        case DISPLAY_REASON_FLAT:
            return "FLT";
        case DISPLAY_REASON_NONE:
        default:
            return "NON";
    }
}

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

void display_show_main_page(const display_view_t *view)
{
    char line[32];
    uint8_t ldr_percent;
    uint8_t output_percent;
    int32_t offset_value;

    if (view == NULL) {
        return;
    }

    ldr_percent = clamp_percent_u8(view->ldr_percent);
    output_percent = clamp_percent_u8(view->output_percent);
    offset_value = clamp_offset_i32(view->manual_offset);

    ssd1306_Fill(Black);

    (void)snprintf(line, sizeof(line), "MODE:%s", mode_to_text(view->mode));
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(line, Font_7x10, White);

    (void)snprintf(line, sizeof(line), "LDR:%3u%%", (unsigned int)ldr_percent);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString(line, Font_7x10, White);
    draw_progress_bar(0U, 24U, 127U, 7U, ldr_percent);

    (void)snprintf(line, sizeof(line), "OUT:%3u%% O:%+ld", (unsigned int)output_percent, (long)offset_value);
    ssd1306_SetCursor(0, 34);
    ssd1306_WriteString(line, Font_7x10, White);
    draw_progress_bar(0U, 46U, 127U, 7U, output_percent);

    draw_page_bullets(0U);
    ssd1306_UpdateScreen();
}

void display_show_sensor_page(const display_view_t *view)
{
    char line[32];

    if (view == NULL) {
        return;
    }

    ssd1306_Fill(Black);

    (void)snprintf(line, sizeof(line), "DIST:%3lu cm", (unsigned long)view->distance_cm);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(line, Font_7x10, White);

    (void)snprintf(line, sizeof(line), "LDRF:%4u", (unsigned int)view->ldr_filtered_raw);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString(line, Font_7x10, White);

    (void)snprintf(line, sizeof(line), "REF:%3lu cm", (unsigned long)view->ref_cm);
    ssd1306_SetCursor(0, 24);
    ssd1306_WriteString(line, Font_7x10, White);

    (void)snprintf(line,
                   sizeof(line),
                   "PRS:%u RSN:%s",
                   (unsigned int)((view->present != 0U) ? 1U : 0U),
                   reason_to_text(view->reason));
    ssd1306_SetCursor(0, 36);
    ssd1306_WriteString(line, Font_7x10, White);

    draw_page_bullets(1U);
    ssd1306_UpdateScreen();
}

void display_show_offset_overlay(int32_t offset)
{
    char line[20];
    uint8_t bar_fill;
    uint8_t center_x = 64U;
    int32_t clamped = clamp_offset_i32(offset);
    uint32_t magnitude = (clamped >= 0) ? (uint32_t)clamped : (uint32_t)(-clamped);

    if (magnitude > 50U) {
        magnitude = 50U;
    }

    bar_fill = (uint8_t)((magnitude * 40U) / 50U);

    ssd1306_Fill(Black);
    ssd1306_SetCursor(24, 2);
    ssd1306_WriteString((char *)"OFFSET", Font_7x10, White);

    (void)snprintf(line, sizeof(line), "%+ld", (long)clamped);
    ssd1306_SetCursor(24, 16);
    ssd1306_WriteString(line, Font_16x24, White);

    ssd1306_Line(22U, 54U, 106U, 54U, White);
    ssd1306_Line(center_x, 50U, center_x, 58U, White);

    if (bar_fill > 0U) {
        if (clamped > 0) {
            ssd1306_FillRectangle((uint8_t)(center_x + 1U), 52U, (uint8_t)(center_x + bar_fill), 56U, White);
        } else {
            ssd1306_FillRectangle((uint8_t)(center_x - bar_fill), 52U, (uint8_t)(center_x - 1U), 56U, White);
        }
    }

    ssd1306_UpdateScreen();
}
