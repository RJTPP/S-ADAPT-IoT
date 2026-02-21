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
} display_view_t;

uint8_t display_init(void);
void display_show_boot(void);
void display_show_distance_cm(uint32_t distance_cm);
void display_show_main_page(const display_view_t *view);
void display_show_sensor_page(const display_view_t *view);
void display_show_offset_overlay(int32_t offset);

#endif /* DISPLAY_H */
