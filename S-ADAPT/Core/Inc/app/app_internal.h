#ifndef APP_INTERNAL_H
#define APP_INTERNAL_H

#include "app/app.h"

#include "support/debug_print.h"
#include "support/filter_utils.h"
#include "bsp/display.h"
#include "bsp/main_led.h"
#include "bsp/status_led.h"
#include "input/encoder_input.h"
#include "input/switch_input.h"
#include "input/input_utils.h"
#include "sensors/ldr.h"
#include "sensors/ultrasonic.h"

typedef struct
{
    uint32_t control_tick_ms;
    uint32_t ldr_sample_ms;
    uint32_t us_sample_ms;
    uint32_t log_ms;
} app_timing_cfg_t;

typedef struct
{
    uint32_t boot_setup_ms;
    uint32_t double_click_ms;
    int32_t offset_step;
    int32_t offset_min;
    int32_t offset_max;
    uint32_t distance_error_cm;
    uint32_t us_timeout_us;
    uint8_t ldr_ma_window_size;
    uint8_t output_hysteresis_band_percent;
    uint8_t output_ramp_step_percent;
    uint8_t output_ramp_step_on_percent;
    uint8_t output_ramp_step_off_percent;
    uint32_t presence_ref_fallback_cm;
    uint32_t presence_body_margin_cm;
    uint32_t presence_return_band_cm;
    uint32_t presence_return_confirm_ms;
    uint32_t presence_away_timeout_ms;
    uint32_t presence_flat_band_cm;
    uint32_t presence_motion_delta_cm;
    uint32_t presence_stale_timeout_ms;
    uint32_t presence_resume_motion_ms;
    uint8_t presence_preoff_dim_percent;
    uint32_t presence_preoff_dim_ms;
    uint32_t ui_overlay_timeout_ms;
    uint32_t ui_refresh_ms;
} app_policy_cfg_t;

typedef struct
{
    uint32_t boot_start_ms;
    uint32_t last_control_tick_ms;
    uint32_t last_ldr_sample_ms;
    uint32_t last_us_sample_ms;
    uint32_t last_log_ms;
    uint32_t last_ui_refresh_ms;
} app_timing_state_t;

typedef struct
{
    uint16_t last_ldr_raw;
    uint16_t last_ldr_filtered;
    ldr_status_t last_ldr_status;

    uint32_t last_distance_raw_cm;
    uint32_t last_distance_filtered_cm;
    uint32_t last_valid_distance_cm;
    ultrasonic_status_t last_us_status;
    uint8_t last_valid_presence;
    uint32_t ref_distance_cm;
    uint8_t ref_valid;
    uint8_t ref_pending_capture;
    uint8_t using_fallback_ref;
    uint32_t prev_valid_distance_cm;
    uint8_t prev_valid_distance_ready;
    uint32_t away_streak_ms;
    uint32_t flat_streak_ms;
    uint32_t motion_streak_ms;
    uint32_t near_ref_streak_ms;
    uint8_t no_user_reason;
    uint8_t presence_candidate_no_user;

    filter_moving_average_u16_t ldr_ma;
    filter_median3_u32_t dist_median3;
} app_sensor_state_t;

typedef struct
{
    uint8_t light_enabled;
    int32_t manual_offset;
    uint8_t auto_percent;
    uint8_t target_output_percent;
    uint8_t hysteresis_output_percent;
    uint8_t ramped_output_percent;
    uint8_t output_percent;
    uint8_t last_applied_output_percent;
    uint8_t output_hysteresis_initialized;
    uint8_t ramp_initialized;
    status_led_state_t rgb_state;
    uint8_t fatal_fault;
    uint8_t preoff_active;
    uint32_t preoff_start_ms;
    uint8_t preoff_dim_target_percent;
} app_control_state_t;

typedef struct
{
    uint8_t pending;
    uint32_t deadline_ms;
    uint32_t last_press_ms;
    uint32_t last_release_ms;
} app_click_state_t;

typedef struct
{
    uint8_t page_index;
    uint8_t page_count;
    uint8_t overlay_active;
    uint32_t overlay_until_ms;
    int32_t overlay_offset;
    uint8_t render_dirty;
} app_ui_state_t;

typedef struct
{
    uint8_t display_ready;
} app_platform_state_t;

typedef struct
{
    app_timing_state_t timing;
    app_sensor_state_t sensors;
    app_control_state_t control;
    app_click_state_t click;
    app_ui_state_t ui;
    app_platform_state_t platform;
} app_ctx_t;

extern const app_timing_cfg_t s_timing_cfg;
extern const app_policy_cfg_t s_policy_cfg;
extern app_ctx_t s_app;

status_led_state_t app_evaluate_state(uint32_t now_ms);
void app_handle_click_timeout(uint32_t now_ms);
void app_handle_encoder_event(const encoder_event_t *event);
void app_process_switch_events(uint32_t now_ms);
void app_process_encoder_events(uint32_t now_ms);
void app_sample_ldr_if_due(uint32_t now_ms);
void app_sample_ultrasonic_if_due(uint32_t now_ms);
uint8_t app_control_tick_due(uint32_t now_ms);
void app_update_output_control(uint32_t now_ms);
void app_update_rgb(uint32_t now_ms);
void app_update_oled_if_due(uint32_t now_ms);
void app_log_summary_if_due(uint32_t now_ms);
const char *status_led_state_to_string(status_led_state_t state);

#endif /* APP_INTERNAL_H */
