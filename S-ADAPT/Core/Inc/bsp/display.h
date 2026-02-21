#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32l4xx_hal.h"

typedef enum
{
    DISPLAY_MODE_OFF = 0,
    DISPLAY_MODE_ON,
    DISPLAY_MODE_SLEEP
} display_mode_t;

typedef enum
{
    DISPLAY_REASON_NONE = 0,
    DISPLAY_REASON_AWAY,
    DISPLAY_REASON_FLAT
} display_reason_t;

typedef enum
{
    DISPLAY_BADGE_NONE = 0,
    DISPLAY_BADGE_LEAVE,
    DISPLAY_BADGE_DIM,
    DISPLAY_BADGE_AWAY,
    DISPLAY_BADGE_IDLE
} display_badge_t;

typedef enum
{
    DISPLAY_SETTINGS_ROW_AWAY_MODE = 0,
    DISPLAY_SETTINGS_ROW_FLAT_MODE,
    DISPLAY_SETTINGS_ROW_AWAY_TIMEOUT,
    DISPLAY_SETTINGS_ROW_FLAT_TIMEOUT,
    DISPLAY_SETTINGS_ROW_PREOFF_DIM,
    DISPLAY_SETTINGS_ROW_RETURN_BAND,
    DISPLAY_SETTINGS_ROW_SAVE,
    DISPLAY_SETTINGS_ROW_RESET,
    DISPLAY_SETTINGS_ROW_EXIT,
    DISPLAY_SETTINGS_ROW_COUNT
} display_settings_row_id_t;

typedef enum
{
    DISPLAY_SETTINGS_STATUS_NONE = 0,
    DISPLAY_SETTINGS_STATUS_SAVED,
    DISPLAY_SETTINGS_STATUS_SAVE_ERR,
    DISPLAY_SETTINGS_STATUS_RESET
} display_settings_status_t;

typedef struct
{
    display_mode_t mode;
    uint8_t ldr_percent;
    uint8_t output_percent;
    int32_t manual_offset;
    uint32_t distance_cm;
    uint16_t ldr_filtered_raw;
    uint32_t ref_cm;
    uint8_t present;
    display_reason_t reason;
    display_badge_t badge;
} display_view_t;

typedef struct
{
    uint8_t selected_row;
    uint8_t row_count;
    uint8_t editing_value;
    uint8_t unsaved;
    uint8_t away_mode_enabled;
    uint8_t flat_mode_enabled;
    uint16_t away_timeout_s;
    uint16_t stale_timeout_s;
    uint16_t preoff_dim_s;
    uint8_t return_band_cm;
    display_settings_status_t status;
} display_settings_view_t;

uint8_t display_init(void);
void display_show_boot(void);
void display_show_main_page(const display_view_t *view);
void display_show_sensor_page(const display_view_t *view);
void display_show_offset_overlay(int32_t offset);
void display_show_settings_page(const display_settings_view_t *view);

#endif /* DISPLAY_H */
