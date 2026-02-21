#include "bsp/display.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>
#include <string.h>

#define DISPLAY_OFFSET_MIN           (-50)
#define DISPLAY_OFFSET_MAX           50
#define DISPLAY_OFFSET_MAGNITUDE_MAX 50U
#define DISPLAY_OFFSET_BAR_HALF_PX   40U
#define DISPLAY_SETTINGS_VISIBLE_ROWS 4U
#define DISPLAY_SETTINGS_ROW_Y_START  12U
#define DISPLAY_SETTINGS_ROW_HEIGHT   10U
#define DISPLAY_SETTINGS_LABEL_X      8U
#define DISPLAY_SETTINGS_VALUE_X      76U
#define DISPLAY_SETTINGS_SCROLL_X0    122U
#define DISPLAY_SETTINGS_SCROLL_X1    125U
#define DISPLAY_SETTINGS_SCROLL_Y0    DISPLAY_SETTINGS_ROW_Y_START
#define DISPLAY_SETTINGS_SCROLL_Y1    (DISPLAY_SETTINGS_ROW_Y_START + (DISPLAY_SETTINGS_VISIBLE_ROWS * DISPLAY_SETTINGS_ROW_HEIGHT) - 1U)

static uint8_t clamp_percent_u8(uint8_t value)
{
    return (value > 100U) ? 100U : value;
}

static int32_t clamp_offset_i32(int32_t value)
{
    if (value < DISPLAY_OFFSET_MIN) {
        return DISPLAY_OFFSET_MIN;
    }
    if (value > DISPLAY_OFFSET_MAX) {
        return DISPLAY_OFFSET_MAX;
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

static const char *badge_to_text(display_badge_t badge)
{
    switch (badge) {
        case DISPLAY_BADGE_LEAVE:
            return "LEAVE";
        case DISPLAY_BADGE_DIM:
            return "DIM";
        case DISPLAY_BADGE_AWAY:
            return "AWAY";
        case DISPLAY_BADGE_IDLE:
            return "IDLE";
        case DISPLAY_BADGE_NONE:
        default:
            return NULL;
    }
}

static void draw_inverted_badge(display_badge_t badge)
{
    const char *text = badge_to_text(badge);
    const uint8_t badge_y = 2U;
    uint8_t text_len;
    uint8_t text_px;
    uint8_t x;

    if (text == NULL) {
        return;
    }

    text_len = (uint8_t)strlen(text);
    text_px = (uint8_t)(text_len * Font_7x10.width);
    if (text_px > 124U) {
        text_px = 124U;
    }

    x = (uint8_t)(SSD1306_WIDTH - text_px - 2U);
    ssd1306_SetCursor(x, badge_y);
    ssd1306_WriteString((char *)text, Font_7x10, White);
    (void)ssd1306_InvertRectangle((uint8_t)(x - 1U), (uint8_t)(badge_y - 2U), (uint8_t)(x + text_px), (uint8_t)(badge_y + 10U));
}

static void draw_boot_frame(const char *status, uint8_t progress_percent)
{
    char line[24];
    const char *title = "S-ADAPT";
    uint8_t title_width = (uint8_t)(strlen(title) * Font_16x24.width);
    uint8_t title_x = 0U;

    ssd1306_Fill(Black);

    if (title_width < SSD1306_WIDTH) {
        title_x = (uint8_t)((SSD1306_WIDTH - title_width) / 2U);
    }

    ssd1306_SetCursor(title_x, 6);
    ssd1306_WriteString((char *)title, Font_16x24, White);

    ssd1306_SetCursor(0, 36);
    (void)snprintf(line, sizeof(line), "%s", status);
    ssd1306_WriteString(line, Font_7x10, White);

    draw_progress_bar(0U, 52U, 127U, 8U, progress_percent);
    ssd1306_UpdateScreen();
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
    draw_boot_frame("Init display...", 25U);
    HAL_Delay(120);
    draw_boot_frame("Init sensors...", 55U);
    HAL_Delay(120);
    draw_boot_frame("Init control...", 85U);
    HAL_Delay(120);
    draw_boot_frame("Ready", 100U);
    HAL_Delay(220);
}

static const char *settings_status_to_text(display_settings_status_t status)
{
    switch (status) {
        case DISPLAY_SETTINGS_STATUS_SAVED:
            return "SAVED";
        case DISPLAY_SETTINGS_STATUS_SAVE_ERR:
            return "SAVE ERR";
        case DISPLAY_SETTINGS_STATUS_RESET:
            return "RESET";
        default:
            return NULL;
    }
}

static void draw_text_token(uint8_t x, uint8_t y, const char *text, uint8_t inverted)
{
    uint8_t text_width;

    if (text == NULL) {
        return;
    }

    ssd1306_SetCursor(x, y);
    ssd1306_WriteString((char *)text, Font_7x10, White);
    if (inverted == 0U) {
        return;
    }

    text_width = (uint8_t)(strlen(text) * Font_7x10.width);
    if (text_width == 0U) {
        return;
    }

    (void)ssd1306_InvertRectangle((uint8_t)(x - 1U), (uint8_t)(y - 1U), (uint8_t)(x + text_width), (uint8_t)(y + 9U));
}

static uint8_t settings_row_has_editable_value(display_settings_row_id_t row)
{
    return (row <= DISPLAY_SETTINGS_ROW_RETURN_BAND) ? 1U : 0U;
}

static void draw_settings_scrollbar(uint8_t row_count, uint8_t row_window_start)
{
    uint8_t track_height;
    uint8_t thumb_height;
    uint8_t thumb_offset = 0U;

    if (row_count <= DISPLAY_SETTINGS_VISIBLE_ROWS) {
        return;
    }

    track_height = (uint8_t)(DISPLAY_SETTINGS_SCROLL_Y1 - DISPLAY_SETTINGS_SCROLL_Y0 + 1U);
    thumb_height = (uint8_t)((track_height * DISPLAY_SETTINGS_VISIBLE_ROWS) / row_count);
    if (thumb_height < 4U) {
        thumb_height = 4U;
    }
    if (thumb_height > track_height) {
        thumb_height = track_height;
    }

    if (row_count > DISPLAY_SETTINGS_VISIBLE_ROWS) {
        uint8_t max_window_start = (uint8_t)(row_count - DISPLAY_SETTINGS_VISIBLE_ROWS);
        uint8_t scroll_range = (uint8_t)(track_height - thumb_height);
        thumb_offset = (uint8_t)((scroll_range * row_window_start) / max_window_start);
    }

    ssd1306_DrawRectangle(DISPLAY_SETTINGS_SCROLL_X0, DISPLAY_SETTINGS_SCROLL_Y0,
                          DISPLAY_SETTINGS_SCROLL_X1, DISPLAY_SETTINGS_SCROLL_Y1, White);
    ssd1306_FillRectangle((uint8_t)(DISPLAY_SETTINGS_SCROLL_X0 + 1U),
                          (uint8_t)(DISPLAY_SETTINGS_SCROLL_Y0 + thumb_offset + 1U),
                          (uint8_t)(DISPLAY_SETTINGS_SCROLL_X1 - 1U),
                          (uint8_t)(DISPLAY_SETTINGS_SCROLL_Y0 + thumb_offset + thumb_height - 2U),
                          White);
}

void display_show_settings_page(const display_settings_view_t *view)
{
    uint8_t selected_row;
    uint8_t row_count;
    uint8_t row_window_start = 0U;
    uint8_t row_idx;
    const char *status_text;

    if (view == NULL) {
        return;
    }

    row_count = view->row_count;
    if ((row_count == 0U) || (row_count > DISPLAY_SETTINGS_ROW_COUNT)) {
        row_count = DISPLAY_SETTINGS_ROW_COUNT;
    }

    selected_row = view->selected_row;
    if (selected_row >= row_count) {
        selected_row = (uint8_t)(row_count - 1U);
    }

    if (selected_row >= DISPLAY_SETTINGS_VISIBLE_ROWS) {
        row_window_start = (uint8_t)(selected_row - (DISPLAY_SETTINGS_VISIBLE_ROWS - 1U));
    }
    if ((row_window_start + DISPLAY_SETTINGS_VISIBLE_ROWS) > row_count) {
        if (row_count > DISPLAY_SETTINGS_VISIBLE_ROWS) {
            row_window_start = (uint8_t)(row_count - DISPLAY_SETTINGS_VISIBLE_ROWS);
        } else {
            row_window_start = 0U;
        }
    }

    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString((char *)"SETTINGS", Font_7x10, White);
    if (view->editing_value != 0U) {
        ssd1306_SetCursor(108, 0);
        ssd1306_WriteString((char *)"E", Font_7x10, White);
    }
    if (view->unsaved != 0U) {
        ssd1306_SetCursor(120, 0);
        ssd1306_WriteString((char *)"*", Font_7x10, White);
    }

    for (row_idx = 0U; row_idx < DISPLAY_SETTINGS_VISIBLE_ROWS; row_idx++) {
        char value_text[12];
        const char *label = "";
        const char *unit = NULL;
        uint8_t current_row = (uint8_t)(row_window_start + row_idx);
        uint8_t y = (uint8_t)(DISPLAY_SETTINGS_ROW_Y_START + (row_idx * DISPLAY_SETTINGS_ROW_HEIGHT));
        uint8_t invert_value = 0U;
        uint8_t invert_label = 0U;

        if (current_row >= row_count) {
            break;
        }

        value_text[0] = '\0';
        if (current_row == selected_row) {
            ssd1306_SetCursor(0, y);
            ssd1306_WriteString((char *)">", Font_7x10, White);
        }

        switch ((display_settings_row_id_t)current_row) {
            case DISPLAY_SETTINGS_ROW_AWAY_MODE:
                label = "Away Mode:";
                (void)snprintf(value_text, sizeof(value_text), "%s", (view->away_mode_enabled != 0U) ? "ON" : "OFF");
                break;
            case DISPLAY_SETTINGS_ROW_FLAT_MODE:
                label = "Flat Mode:";
                (void)snprintf(value_text, sizeof(value_text), "%s", (view->flat_mode_enabled != 0U) ? "ON" : "OFF");
                break;
            case DISPLAY_SETTINGS_ROW_AWAY_TIMEOUT:
                label = "Away T:";
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned int)view->away_timeout_s);
                unit = "s";
                break;
            case DISPLAY_SETTINGS_ROW_FLAT_TIMEOUT:
                label = "Flat T:";
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned int)view->stale_timeout_s);
                unit = "s";
                break;
            case DISPLAY_SETTINGS_ROW_PREOFF_DIM:
                label = "PreOff:";
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned int)view->preoff_dim_s);
                unit = "s";
                break;
            case DISPLAY_SETTINGS_ROW_RETURN_BAND:
                label = "RetBand:";
                (void)snprintf(value_text, sizeof(value_text), "%u", (unsigned int)view->return_band_cm);
                unit = "cm";
                break;
            case DISPLAY_SETTINGS_ROW_SAVE:
                label = "Save";
                break;
            case DISPLAY_SETTINGS_ROW_RESET:
                label = "Reset";
                break;
            case DISPLAY_SETTINGS_ROW_EXIT:
            default:
                if ((view->unsaved != 0U) && (current_row == selected_row)) {
                    label = "Exit?";
                    invert_label = 1U;
                } else {
                    label = "Exit";
                }
                break;
        }

        if (invert_label != 0U) {
            draw_text_token(DISPLAY_SETTINGS_LABEL_X, y, label, 1U);
        } else {
            ssd1306_SetCursor(DISPLAY_SETTINGS_LABEL_X, y);
            ssd1306_WriteString((char *)label, Font_7x10, White);
        }

        if ((view->editing_value != 0U) &&
            (current_row == selected_row) &&
            (settings_row_has_editable_value((display_settings_row_id_t)current_row) != 0U)) {
            invert_value = 1U;
        }

        if (value_text[0] != '\0') {
            uint8_t value_x = DISPLAY_SETTINGS_VALUE_X;
            draw_text_token(value_x, y, value_text, invert_value);
            if (unit != NULL) {
                uint8_t unit_x = (uint8_t)(value_x + (strlen(value_text) * Font_7x10.width) + Font_7x10.width);
                ssd1306_SetCursor(unit_x, y);
                ssd1306_WriteString((char *)unit, Font_7x10, White);
            }
        }
    }

    status_text = settings_status_to_text(view->status);
    if (status_text != NULL) {
        ssd1306_SetCursor(72, 54);
        ssd1306_WriteString((char *)status_text, Font_7x10, White);
    }

    draw_settings_scrollbar(row_count, row_window_start);

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
    draw_inverted_badge(view->badge);

    (void)snprintf(line, sizeof(line), "LDR:%3u%%", (unsigned int)ldr_percent);
    ssd1306_SetCursor(0, 12);
    ssd1306_WriteString(line, Font_7x10, White);
    draw_progress_bar(0U, 24U, 127U, 7U, ldr_percent);

    (void)snprintf(line, sizeof(line), "OUT:%3u%% OFF:%+ld", (unsigned int)output_percent, (long)offset_value);
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

    bar_fill = (uint8_t)((magnitude * DISPLAY_OFFSET_BAR_HALF_PX) / DISPLAY_OFFSET_MAGNITUDE_MAX);

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
