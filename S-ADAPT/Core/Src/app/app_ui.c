#include "app/app_internal.h"

#define APP_ADC_MAX_VALUE 4095U

typedef struct
{
    uint8_t valid;
    uint8_t overlay_active;
    uint8_t page_index;
    int32_t overlay_offset;
    display_view_t view;
} app_ui_snapshot_t;

static app_ui_snapshot_t s_ui_snapshot;

static const char *no_user_reason_to_string(uint8_t reason)
{
    switch (reason) {
        case 1U:
            return "away";
        case 2U:
            return "flat";
        default:
            return "none";
    }
}

static display_mode_t app_to_display_mode(void)
{
    if (s_app.control.light_enabled == 0U) {
        return DISPLAY_MODE_OFF;
    }
    if (s_app.sensors.last_valid_presence == 0U) {
        return DISPLAY_MODE_SLEEP;
    }
    return DISPLAY_MODE_ON;
}

static display_reason_t app_to_display_reason(void)
{
    switch (s_app.sensors.no_user_reason) {
        case 1U:
            return DISPLAY_REASON_AWAY;
        case 2U:
            return DISPLAY_REASON_FLAT;
        default:
            return DISPLAY_REASON_NONE;
    }
}

static uint8_t app_compute_ldr_percent(uint16_t ldr_filtered_raw)
{
    uint32_t scaled = ((uint32_t)ldr_filtered_raw * 100U) / APP_ADC_MAX_VALUE;
    if (scaled > 100U) {
        scaled = 100U;
    }
    return (uint8_t)scaled;
}

static display_badge_t app_select_main_badge(void)
{
    if (s_app.sensors.away_streak_ms > 0U) {
        return DISPLAY_BADGE_LEAVE;
    }

    if (s_app.control.preoff_active != 0U) {
        return DISPLAY_BADGE_DIM;
    }

    if ((s_app.sensors.presence_candidate_no_user != 0U) ||
        (s_app.sensors.last_valid_presence == 0U)) {
        return DISPLAY_BADGE_LEAVE;
    }

    return DISPLAY_BADGE_NONE;
}

static void app_compose_display_view(display_view_t *view)
{
    if (view == NULL) {
        return;
    }

    view->mode = app_to_display_mode();
    view->ldr_percent = app_compute_ldr_percent(s_app.sensors.last_ldr_filtered);
    view->output_percent = s_app.control.output_percent;
    view->manual_offset = s_app.control.manual_offset;
    view->distance_cm = s_app.sensors.last_valid_distance_cm;
    view->ldr_filtered_raw = s_app.sensors.last_ldr_filtered;
    view->ref_cm = s_app.sensors.ref_distance_cm;
    view->present = s_app.sensors.last_valid_presence;
    view->reason = app_to_display_reason();
    view->badge = app_select_main_badge();
}

static uint8_t app_view_changed_for_page(const display_view_t *last_view,
                                         const display_view_t *current_view,
                                         uint8_t page_index)
{
    if ((last_view == NULL) || (current_view == NULL)) {
        return 1U;
    }

    if (page_index == 0U) {
        if (last_view->mode != current_view->mode) {
            return 1U;
        }
        if (last_view->ldr_percent != current_view->ldr_percent) {
            return 1U;
        }
        if (last_view->output_percent != current_view->output_percent) {
            return 1U;
        }
        if (last_view->manual_offset != current_view->manual_offset) {
            return 1U;
        }
        if (last_view->badge != current_view->badge) {
            return 1U;
        }
        return 0U;
    }

    if (last_view->distance_cm != current_view->distance_cm) {
        return 1U;
    }
    if (last_view->ldr_filtered_raw != current_view->ldr_filtered_raw) {
        return 1U;
    }
    if (last_view->ref_cm != current_view->ref_cm) {
        return 1U;
    }
    if (last_view->present != current_view->present) {
        return 1U;
    }
    if (last_view->reason != current_view->reason) {
        return 1U;
    }

    return 0U;
}

static void app_update_ui_dirty_from_data(const display_view_t *current_view)
{
    if (s_app.ui.overlay_active != 0U) {
        if ((s_ui_snapshot.valid == 0U) ||
            (s_ui_snapshot.overlay_active == 0U) ||
            (s_ui_snapshot.overlay_offset != s_app.ui.overlay_offset)) {
            s_app.ui.render_dirty = 1U;
        }
        return;
    }

    if (s_ui_snapshot.valid == 0U) {
        s_app.ui.render_dirty = 1U;
        return;
    }

    if (s_ui_snapshot.overlay_active != 0U) {
        s_app.ui.render_dirty = 1U;
        return;
    }

    if (s_ui_snapshot.page_index != s_app.ui.page_index) {
        s_app.ui.render_dirty = 1U;
        return;
    }

    if (app_view_changed_for_page(&s_ui_snapshot.view, current_view, s_app.ui.page_index) != 0U) {
        s_app.ui.render_dirty = 1U;
    }
}

static void app_snapshot_commit(const display_view_t *current_view)
{
    if (current_view != NULL) {
        s_ui_snapshot.view = *current_view;
    }
    s_ui_snapshot.valid = 1U;
    s_ui_snapshot.overlay_active = s_app.ui.overlay_active;
    s_ui_snapshot.page_index = s_app.ui.page_index;
    s_ui_snapshot.overlay_offset = s_app.ui.overlay_offset;
}

static void app_render_display(const display_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (s_app.ui.overlay_active != 0U) {
        display_show_offset_overlay(s_app.ui.overlay_offset);
        return;
    }

    if (s_app.ui.page_index == 0U) {
        display_show_main_page(view);
    } else {
        display_show_sensor_page(view);
    }
}

const char *status_led_state_to_string(status_led_state_t state)
{
    switch (state) {
        case STATUS_LED_STATE_BOOT_SETUP:
            return "boot_setup";
        case STATUS_LED_STATE_LIGHT_OFF:
            return "light_off";
        case STATUS_LED_STATE_AUTO:
            return "auto";
        case STATUS_LED_STATE_OFFSET_POSITIVE:
            return "offset_positive";
        case STATUS_LED_STATE_NO_USER:
            return "no_user";
        case STATUS_LED_STATE_FAULT_FATAL:
            return "fault_fatal";
        default:
            return "unknown";
    }
}

void app_update_oled_if_due(uint32_t now_ms)
{
    display_view_t current_view;
    uint8_t redraw_rate_limited = 0U;
    uint8_t periodic_due = 0U;

    if (s_app.platform.display_ready == 0U) {
        return;
    }

    if ((s_app.ui.overlay_active != 0U) &&
        ((int32_t)(now_ms - s_app.ui.overlay_until_ms) >= 0)) {
        s_app.ui.overlay_active = 0U;
        s_app.ui.render_dirty = 1U;
    }

    app_compose_display_view(&current_view);
    app_update_ui_dirty_from_data(&current_view);
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_ui_draw_ms, s_timing_cfg.ui_min_redraw_ms) == 0U) {
        redraw_rate_limited = 1U;
    }

    if (input_has_elapsed_ms(now_ms, s_app.timing.last_ui_refresh_ms, s_policy_cfg.ui_refresh_ms) != 0U) {
        s_app.timing.last_ui_refresh_ms += s_policy_cfg.ui_refresh_ms;
        periodic_due = 1U;
    }

    if ((s_app.ui.render_dirty == 0U) && (periodic_due == 0U)) {
        return;
    }

    if ((periodic_due == 0U) && (redraw_rate_limited != 0U)) {
        return;
    }

    app_render_display(&current_view);
    s_app.timing.last_ui_draw_ms = now_ms;
    app_snapshot_commit(&current_view);
    s_app.ui.render_dirty = 0U;
}

void app_log_summary_if_due(uint32_t now_ms)
{
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_log_ms, s_timing_cfg.log_ms) != 0U) {
        uint32_t preoff_ms = 0U;

        s_app.timing.last_log_ms = now_ms;
        if (s_app.control.preoff_active != 0U) {
            preoff_ms = (uint32_t)(now_ms - s_app.control.preoff_start_ms);
        }

        debug_logln(DEBUG_PRINT_INFO,
                    "dbg summary ldr_raw=%u ldr_filt=%u ldr_status=%s dist_cm_raw_last_valid=%lu dist_cm_filt=%lu us_status=%s present=%u light_on=%u offset=%ld auto=%u target_out=%u hyst_out=%u applied_out=%u ref_cm=%lu ref_src=%s away_ms=%lu flat_ms=%lu motion_ms=%lu no_user_reason=%s preoff=%u preoff_ms=%lu preoff_target=%u rgb=%s",
                    (unsigned int)s_app.sensors.last_ldr_raw,
                    (unsigned int)s_app.sensors.last_ldr_filtered,
                    ldr_status_to_string(s_app.sensors.last_ldr_status),
                    (unsigned long)s_app.sensors.last_distance_raw_cm,
                    (unsigned long)s_app.sensors.last_distance_filtered_cm,
                    ultrasonic_status_to_string(s_app.sensors.last_us_status),
                    (unsigned int)s_app.sensors.last_valid_presence,
                    (unsigned int)s_app.control.light_enabled,
                    (long)s_app.control.manual_offset,
                    (unsigned int)s_app.control.auto_percent,
                    (unsigned int)s_app.control.target_output_percent,
                    (unsigned int)s_app.control.hysteresis_output_percent,
                    (unsigned int)s_app.control.output_percent,
                    (unsigned long)s_app.sensors.ref_distance_cm,
                    (s_app.sensors.using_fallback_ref != 0U) ? "fallback" : "captured",
                    (unsigned long)s_app.sensors.away_streak_ms,
                    (unsigned long)s_app.sensors.flat_streak_ms,
                    (unsigned long)s_app.sensors.motion_streak_ms,
                    no_user_reason_to_string(s_app.sensors.no_user_reason),
                    (unsigned int)s_app.control.preoff_active,
                    (unsigned long)preoff_ms,
                    (unsigned int)s_app.control.preoff_dim_target_percent,
                    status_led_state_to_string(s_app.control.rgb_state));
    }
}
