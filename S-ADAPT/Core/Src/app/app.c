#include "app/app_internal.h"

#ifndef APP_ENABLE_DISPLAY
#define APP_ENABLE_DISPLAY 1U
#endif

/* Bring-up aid: when enabled, shorten presence/pre-off timers for faster hardware testing. */
/* Set to 0U for production timing behavior. */
#ifndef APP_PRESENCE_DEBUG_TIMERS
#define APP_PRESENCE_DEBUG_TIMERS 1U
#endif

#if APP_PRESENCE_DEBUG_TIMERS
#define APP_PRESENCE_AWAY_TIMEOUT_MS 5000U
#define APP_PRESENCE_STALE_TIMEOUT_MS 15000U
#define APP_PRESENCE_RESUME_MOTION_MS 2000U
#define APP_PRESENCE_PREOFF_DIM_MS 5000U
#else
#define APP_PRESENCE_AWAY_TIMEOUT_MS 30000U
#define APP_PRESENCE_STALE_TIMEOUT_MS 120000U
#define APP_PRESENCE_RESUME_MOTION_MS 5000U
#define APP_PRESENCE_PREOFF_DIM_MS 10000U
#endif

const app_timing_cfg_t s_timing_cfg = {
    .control_tick_ms = 33U,
    .ldr_sample_ms = 50U,
    .us_sample_ms = 100U,
    .log_ms = 1000U,
    .ui_min_redraw_ms = 66U,
};

const app_policy_cfg_t s_policy_cfg = {
    .boot_setup_ms = 1000U,
    .double_click_ms = 350U,
    .offset_step = 5,
    .offset_min = -50,
    .offset_max = 50,
    .distance_error_cm = 999U,
    .us_timeout_us = 30000U,
    .ldr_ma_window_size = 8U,
    .output_hysteresis_band_percent = 5U,
    .output_ramp_step_percent = 1U,
    .output_ramp_step_on_percent = 3U,
    .output_ramp_step_off_percent = 5U,
    .presence_ref_fallback_cm = 60U,
    .presence_body_margin_cm = 20U,
    .presence_return_band_cm = 10U,
    .presence_return_confirm_ms = 1500U,
    .presence_away_timeout_ms = APP_PRESENCE_AWAY_TIMEOUT_MS,
    .presence_flat_band_cm = 1U,
    .presence_motion_delta_cm = 2U,
    .presence_stale_timeout_ms = APP_PRESENCE_STALE_TIMEOUT_MS,
    .presence_resume_motion_ms = APP_PRESENCE_RESUME_MOTION_MS,
    .presence_preoff_dim_percent = 15U,
    .presence_preoff_dim_ms = APP_PRESENCE_PREOFF_DIM_MS,
    .ui_overlay_timeout_ms = 1200U,
    .ui_refresh_ms = 1000U,
};

app_ctx_t s_app;

void app_set_fatal_fault(uint8_t enabled)
{
    s_app.control.fatal_fault = (enabled != 0U) ? 1U : 0U;
    status_led_set_fatal_fault(s_app.control.fatal_fault);
}

uint8_t app_init(const app_hw_config_t *hw)
{
    uint32_t now_ms = HAL_GetTick();
    uint8_t ok = 1U;
    main_led_status_t main_led_status;

    if ((hw == NULL) || (hw->ldr_adc == NULL) || (hw->echo_tim == NULL) || (hw->main_led_tim == NULL)) {
        s_app.control.fatal_fault = 1U;
        debug_logln(DEBUG_PRINT_ERROR, "app init invalid hw config");
        return 0U;
    }

    s_app.timing.boot_start_ms = now_ms;
    s_app.timing.last_control_tick_ms = now_ms;
    s_app.timing.last_ldr_sample_ms = now_ms;
    s_app.timing.last_us_sample_ms = now_ms;
    s_app.timing.last_log_ms = now_ms;
    s_app.timing.last_ui_draw_ms = now_ms;
    s_app.timing.last_ui_refresh_ms = now_ms;

    s_app.sensors.last_ldr_raw = 0U;
    s_app.sensors.last_ldr_filtered = 0U;
    s_app.sensors.last_ldr_status = LDR_STATUS_NOT_INIT;

    s_app.sensors.last_distance_raw_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_distance_filtered_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_valid_distance_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_us_status = ULTRASONIC_STATUS_NOT_INIT;
    s_app.sensors.last_valid_presence = 1U;
    s_app.sensors.ref_distance_cm = s_policy_cfg.presence_ref_fallback_cm;
    s_app.sensors.ref_valid = 0U;
    s_app.sensors.ref_pending_capture = 0U;
    s_app.sensors.using_fallback_ref = 1U;
    s_app.sensors.prev_valid_distance_cm = 0U;
    s_app.sensors.prev_valid_distance_ready = 0U;
    s_app.sensors.away_streak_ms = 0U;
    s_app.sensors.flat_streak_ms = 0U;
    s_app.sensors.motion_streak_ms = 0U;
    s_app.sensors.near_ref_streak_ms = 0U;
    s_app.sensors.no_user_reason = APP_NO_USER_REASON_NONE;
    s_app.sensors.presence_candidate_no_user = 0U;

    filter_moving_average_u16_init(&s_app.sensors.ldr_ma, s_policy_cfg.ldr_ma_window_size);
    filter_median3_u32_init(&s_app.sensors.dist_median3);

    s_app.control.light_enabled = 0U;
    s_app.control.manual_offset = 0;
    s_app.control.auto_percent = 0U;
    s_app.control.target_output_percent = 0U;
    s_app.control.hysteresis_output_percent = 0U;
    s_app.control.ramped_output_percent = 0U;
    s_app.control.output_percent = 0U;
    s_app.control.last_applied_output_percent = 0U;
    s_app.control.output_hysteresis_initialized = 0U;
    s_app.control.ramp_initialized = 0U;
    s_app.control.ramp_fast_on_active = 0U;
    s_app.control.fatal_fault = 0U;
    s_app.control.rgb_state = STATUS_LED_STATE_BOOT_SETUP;
    s_app.control.preoff_active = 0U;
    s_app.control.preoff_start_ms = 0U;
    s_app.control.preoff_dim_target_percent = 0U;

    s_app.click.pending = 0U;
    s_app.click.deadline_ms = now_ms;
    s_app.click.last_press_ms = now_ms;
    s_app.click.last_release_ms = now_ms;

    s_app.ui.page_index = 0U;
    s_app.ui.page_count = 2U;
    s_app.ui.overlay_active = 0U;
    s_app.ui.overlay_until_ms = 0U;
    s_app.ui.overlay_offset = 0;
    s_app.ui.render_dirty = 1U;

    s_app.platform.display_ready = 0U;

    ldr_init(hw->ldr_adc);
    ultrasonic_init(hw->echo_tim, hw->echo_channel);
    switch_input_init();
    encoder_input_init();
    status_led_init();

    main_led_init(hw->main_led_tim, hw->main_led_channel);
    main_led_status = main_led_start();
    debug_logln(DEBUG_PRINT_INFO, "dbg main_led start=%s", main_led_status_to_string(main_led_status));
    if (main_led_status != MAIN_LED_STATUS_OK) {
        ok = 0U;
    }

    main_led_status = main_led_set_enabled(1U);
    debug_logln(DEBUG_PRINT_INFO, "dbg main_led enable=%s", main_led_status_to_string(main_led_status));
    if (main_led_status != MAIN_LED_STATUS_OK) {
        ok = 0U;
    }

    main_led_status = main_led_set_percent(0U);
    debug_logln(DEBUG_PRINT_INFO, "dbg main_led duty=%u status=%s",
                (unsigned int)main_led_get_percent(),
                main_led_status_to_string(main_led_status));
    if (main_led_status != MAIN_LED_STATUS_OK) {
        ok = 0U;
    }

    if (ok == 0U) {
        app_set_fatal_fault(1U);
        debug_logln(DEBUG_PRINT_ERROR, "app init fault -> fatal fault enabled");
    }

#if APP_ENABLE_DISPLAY
    if (display_init() != 0U) {
        s_app.platform.display_ready = 1U;
        display_show_boot();
        debug_logln(DEBUG_PRINT_INFO, "dbg oled=ready");
    } else {
        s_app.platform.display_ready = 0U;
        debug_logln(DEBUG_PRINT_ERROR, "dbg oled=init_failed");
    }
#else
    s_app.platform.display_ready = 0U;
    debug_logln(DEBUG_PRINT_INFO, "dbg oled=disabled");
#endif

    s_app.control.rgb_state = app_evaluate_state(now_ms);
    status_led_set_state(s_app.control.rgb_state);
    debug_logln(DEBUG_PRINT_INFO, "dbg rgb_state=%s", status_led_state_to_string(s_app.control.rgb_state));

    return ok;
}

void app_step(void)
{
    uint32_t now_ms = HAL_GetTick();

    status_led_tick(now_ms);
    app_process_switch_events(now_ms);
    app_process_encoder_events(now_ms);
    app_handle_click_timeout(now_ms);
    app_sample_ldr_if_due(now_ms);
    app_sample_ultrasonic_if_due(now_ms);

    if (app_control_tick_due(now_ms) == 0U) {
        return;
    }

    app_update_output_control(now_ms);
    app_update_rgb(now_ms);
    app_update_oled_if_due(now_ms);
    app_log_summary_if_due(now_ms);
}
