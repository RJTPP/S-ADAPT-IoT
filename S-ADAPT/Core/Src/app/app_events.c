#include "app/app_internal.h"

#define APP_SETTINGS_LONG_PRESS_MS 1000U
#define APP_SETTINGS_TOAST_MS      1500U

static const char *switch_name(switch_input_id_t input)
{
    return (input == SWITCH_INPUT_BUTTON) ? "BUTTON" : "SW2";
}

static const char *encoder_event_name(encoder_event_type_t type)
{
    switch (type) {
        case ENCODER_EVENT_CW:
            return "cw";
        case ENCODER_EVENT_CCW:
            return "ccw";
        case ENCODER_EVENT_SW_PRESSED:
            return "sw_pressed";
        case ENCODER_EVENT_SW_RELEASED:
            return "sw_released";
        default:
            return "unknown";
    }
}

static int32_t clamp_offset(int32_t offset)
{
    if (offset < s_policy_cfg.offset_min) {
        return s_policy_cfg.offset_min;
    }
    if (offset > s_policy_cfg.offset_max) {
        return s_policy_cfg.offset_max;
    }

    return offset;
}

static uint16_t clamp_settings_u16(uint16_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint16_t wrap_settings_u16(uint16_t current, int8_t direction, uint16_t min_value, uint16_t max_value, uint16_t step)
{
    int32_t next;

    if ((step == 0U) || (max_value <= min_value)) {
        return clamp_settings_u16(current, min_value, max_value);
    }

    next = (int32_t)current + ((int32_t)direction * (int32_t)step);
    if (next > (int32_t)max_value) {
        return min_value;
    }
    if (next < (int32_t)min_value) {
        return max_value;
    }

    return (uint16_t)next;
}

static uint8_t settings_are_equal(const app_settings_t *lhs, const app_settings_t *rhs)
{
    if ((lhs == NULL) || (rhs == NULL)) {
        return 0U;
    }

    if (lhs->away_mode_enabled != rhs->away_mode_enabled) {
        return 0U;
    }
    if (lhs->flat_mode_enabled != rhs->flat_mode_enabled) {
        return 0U;
    }
    if (lhs->away_timeout_s != rhs->away_timeout_s) {
        return 0U;
    }
    if (lhs->stale_timeout_s != rhs->stale_timeout_s) {
        return 0U;
    }
    if (lhs->preoff_dim_s != rhs->preoff_dim_s) {
        return 0U;
    }
    if (lhs->return_band_cm != rhs->return_band_cm) {
        return 0U;
    }

    return 1U;
}

static void app_refresh_settings_dirty(void)
{
    s_app.settings.dirty = (settings_are_equal(&s_app.settings.draft, &s_app.settings.active) != 0U) ? 0U : 1U;
}

static void app_set_settings_toast(app_settings_toast_t toast, uint32_t now_ms)
{
    s_app.settings_ui.toast = toast;
    if (toast == APP_SETTINGS_TOAST_NONE) {
        s_app.settings_ui.toast_until_ms = 0U;
    } else {
        s_app.settings_ui.toast_until_ms = now_ms + APP_SETTINGS_TOAST_MS;
    }
}

static void app_enter_settings_mode(uint32_t now_ms)
{
    s_app.settings_ui.mode_active = 1U;
    s_app.settings_ui.editing_value = 0U;
    s_app.settings_ui.selected_row = 0U;
    s_app.settings_ui.button_pressed = 0U;
    s_app.settings_ui.long_press_fired = 0U;
    s_app.settings_ui.button_press_start_ms = now_ms;
    app_set_settings_toast(APP_SETTINGS_TOAST_NONE, now_ms);
    s_app.settings.draft = s_app.settings.active;
    s_app.settings.dirty = 0U;
    s_app.ui.overlay_active = 0U;
    s_app.ui.render_dirty = 1U;
    debug_logln(DEBUG_PRINT_INFO, "dbg settings=enter");
}

static void app_exit_settings_mode_discard(uint32_t now_ms)
{
    s_app.settings_ui.mode_active = 0U;
    s_app.settings_ui.editing_value = 0U;
    s_app.settings_ui.selected_row = 0U;
    s_app.settings_ui.button_pressed = 0U;
    s_app.settings_ui.long_press_fired = 0U;
    s_app.settings_ui.button_press_start_ms = now_ms;
    app_set_settings_toast(APP_SETTINGS_TOAST_NONE, now_ms);
    s_app.settings.draft = s_app.settings.active;
    s_app.settings.dirty = 0U;
    s_app.ui.render_dirty = 1U;
    debug_logln(DEBUG_PRINT_INFO, "dbg settings=exit discard");
}

static void app_settings_adjust_value(display_settings_row_id_t row, int8_t direction)
{
    switch (row) {
        case DISPLAY_SETTINGS_ROW_AWAY_TIMEOUT:
            s_app.settings.draft.away_timeout_s = wrap_settings_u16(
                s_app.settings.draft.away_timeout_s,
                direction,
                APP_SETTINGS_AWAY_TIMEOUT_S_MIN,
                APP_SETTINGS_AWAY_TIMEOUT_S_MAX,
                APP_SETTINGS_AWAY_TIMEOUT_S_STEP);
            break;
        case DISPLAY_SETTINGS_ROW_FLAT_TIMEOUT:
            s_app.settings.draft.stale_timeout_s = wrap_settings_u16(
                s_app.settings.draft.stale_timeout_s,
                direction,
                APP_SETTINGS_STALE_TIMEOUT_S_MIN,
                APP_SETTINGS_STALE_TIMEOUT_S_MAX,
                APP_SETTINGS_STALE_TIMEOUT_S_STEP);
            break;
        case DISPLAY_SETTINGS_ROW_PREOFF_DIM:
            s_app.settings.draft.preoff_dim_s = wrap_settings_u16(
                s_app.settings.draft.preoff_dim_s,
                direction,
                APP_SETTINGS_PREOFF_DIM_S_MIN,
                APP_SETTINGS_PREOFF_DIM_S_MAX,
                APP_SETTINGS_PREOFF_DIM_S_STEP);
            break;
        case DISPLAY_SETTINGS_ROW_RETURN_BAND:
            s_app.settings.draft.return_band_cm = (uint8_t)wrap_settings_u16(
                s_app.settings.draft.return_band_cm,
                direction,
                APP_SETTINGS_RETURN_BAND_CM_MIN,
                APP_SETTINGS_RETURN_BAND_CM_MAX,
                APP_SETTINGS_RETURN_BAND_CM_STEP);
            break;
        default:
            break;
    }
}

static void app_settings_cycle_row(int8_t direction)
{
    uint8_t selected = s_app.settings_ui.selected_row;

    if (direction > 0) {
        selected = (uint8_t)((selected + 1U) % DISPLAY_SETTINGS_ROW_COUNT);
    } else {
        selected = (selected == 0U) ? (DISPLAY_SETTINGS_ROW_COUNT - 1U) : (uint8_t)(selected - 1U);
    }

    s_app.settings_ui.selected_row = selected;
}

static void reset_presence_runtime_state(void)
{
    s_app.sensors.away_streak_ms = 0U;
    s_app.sensors.flat_streak_ms = 0U;
    s_app.sensors.motion_streak_ms = 0U;
    s_app.sensors.near_ref_streak_ms = 0U;
    s_app.sensors.no_user_reason = APP_NO_USER_REASON_NONE;
    s_app.sensors.presence_candidate_no_user = 0U;
    s_app.control.preoff_active = 0U;
    s_app.control.preoff_start_ms = 0U;
    s_app.control.preoff_dim_target_percent = 0U;
}

static void app_toggle_light_enabled(void)
{
    uint8_t was_light_enabled;

    was_light_enabled = s_app.control.light_enabled;
    s_app.control.light_enabled = (s_app.control.light_enabled == 0U) ? 1U : 0U;

    if ((was_light_enabled == 0U) && (s_app.control.light_enabled != 0U)) {
        s_app.control.ramp_fast_on_active = 1U;
        reset_presence_runtime_state();
        s_app.sensors.ref_distance_cm = s_policy_cfg.presence_ref_fallback_cm;
        s_app.sensors.ref_valid = 1U;
        s_app.sensors.ref_pending_capture = 1U;
        s_app.sensors.using_fallback_ref = 1U;
        s_app.sensors.prev_valid_distance_ready = 0U;
        s_app.sensors.last_valid_presence = 1U;
    } else if ((was_light_enabled != 0U) && (s_app.control.light_enabled == 0U)) {
        s_app.control.ramp_fast_on_active = 0U;
        reset_presence_runtime_state();
    }

    s_app.ui.render_dirty = 1U;
    debug_logln(DEBUG_PRINT_INFO, "dbg click=single light_on=%u", (unsigned int)s_app.control.light_enabled);
}

void app_handle_encoder_event(const encoder_event_t *event)
{
    if (event == NULL) {
        return;
    }

    if (s_app.settings_ui.mode_active != 0U) {
        if ((event->type == ENCODER_EVENT_CW) || (event->type == ENCODER_EVENT_CCW)) {
            int8_t direction = (event->type == ENCODER_EVENT_CW) ? 1 : -1;
            display_settings_row_id_t selected_row = (display_settings_row_id_t)s_app.settings_ui.selected_row;

            if (s_app.settings_ui.editing_value != 0U) {
                app_settings_adjust_value(selected_row, direction);
                app_refresh_settings_dirty();
            } else {
                app_settings_cycle_row(direction);
            }
            s_app.ui.render_dirty = 1U;
            return;
        }

        if (event->type != ENCODER_EVENT_SW_RELEASED) {
            return;
        }

        if (s_app.settings_ui.editing_value != 0U) {
            s_app.settings_ui.editing_value = 0U;
            s_app.ui.render_dirty = 1U;
            return;
        }

        switch ((display_settings_row_id_t)s_app.settings_ui.selected_row) {
            case DISPLAY_SETTINGS_ROW_AWAY_MODE:
                s_app.settings.draft.away_mode_enabled = (s_app.settings.draft.away_mode_enabled == 0U) ? 1U : 0U;
                app_refresh_settings_dirty();
                break;
            case DISPLAY_SETTINGS_ROW_FLAT_MODE:
                s_app.settings.draft.flat_mode_enabled = (s_app.settings.draft.flat_mode_enabled == 0U) ? 1U : 0U;
                app_refresh_settings_dirty();
                break;
            case DISPLAY_SETTINGS_ROW_AWAY_TIMEOUT:
            case DISPLAY_SETTINGS_ROW_FLAT_TIMEOUT:
            case DISPLAY_SETTINGS_ROW_PREOFF_DIM:
            case DISPLAY_SETTINGS_ROW_RETURN_BAND:
                s_app.settings_ui.editing_value = 1U;
                break;
            case DISPLAY_SETTINGS_ROW_SAVE:
            {
                settings_store_status_t save_status;
                app_settings_t validated = s_app.settings.draft;

                (void)app_settings_validate(&validated);
                save_status = settings_store_save(&validated);
                if (save_status == SETTINGS_STORE_OK) {
                    s_app.settings.active = validated;
                    s_app.settings.draft = validated;
                    s_app.settings.dirty = 0U;
                    app_set_settings_toast(APP_SETTINGS_TOAST_SAVED, event->timestamp_ms);
                    debug_logln(DEBUG_PRINT_INFO, "dbg settings=save ok");
                } else {
                    app_set_settings_toast(APP_SETTINGS_TOAST_SAVE_ERR, event->timestamp_ms);
                    debug_logln(DEBUG_PRINT_ERROR, "dbg settings=save err status=%u", (unsigned int)save_status);
                }
                break;
            }
            case DISPLAY_SETTINGS_ROW_RESET:
                app_settings_apply_build_defaults(&s_app.settings.draft);
                app_refresh_settings_dirty();
                app_set_settings_toast(APP_SETTINGS_TOAST_RESET, event->timestamp_ms);
                debug_logln(DEBUG_PRINT_INFO, "dbg settings=reset draft");
                break;
            case DISPLAY_SETTINGS_ROW_EXIT:
                app_exit_settings_mode_discard(event->timestamp_ms);
                break;
            default:
                break;
        }

        s_app.ui.render_dirty = 1U;
        return;
    }

    switch (event->type) {
        case ENCODER_EVENT_CW:
            if (s_app.control.light_enabled != 0U) {
                s_app.control.manual_offset = clamp_offset(s_app.control.manual_offset + s_policy_cfg.offset_step);
            }
            break;

        case ENCODER_EVENT_CCW:
            if (s_app.control.light_enabled != 0U) {
                s_app.control.manual_offset = clamp_offset(s_app.control.manual_offset - s_policy_cfg.offset_step);
            }
            break;

        case ENCODER_EVENT_SW_PRESSED:
            s_app.click.last_press_ms = event->timestamp_ms;
            break;

        case ENCODER_EVENT_SW_RELEASED:
            s_app.click.last_release_ms = event->timestamp_ms;
            if ((uint32_t)(event->timestamp_ms - s_app.click.last_press_ms) >= s_policy_cfg.encoder_long_press_ms) {
                s_app.control.manual_offset = 0;
                s_app.ui.render_dirty = 1U;
                debug_logln(DEBUG_PRINT_INFO, "dbg click=long offset=%ld", (long)s_app.control.manual_offset);
            } else {
                app_toggle_light_enabled();
            }
            break;

        default:
            break;
    }

    if (((event->type == ENCODER_EVENT_CW) || (event->type == ENCODER_EVENT_CCW)) &&
        (s_app.control.light_enabled != 0U)) {
        s_app.ui.overlay_active = 1U;
        s_app.ui.overlay_until_ms = event->timestamp_ms + s_policy_cfg.ui_overlay_timeout_ms;
        s_app.ui.overlay_offset = s_app.control.manual_offset;
        s_app.ui.render_dirty = 1U;
    }
}

void app_process_switch_events(uint32_t now_ms)
{
    switch_input_event_t switch_event;

    switch_input_tick(now_ms);
    while (switch_input_pop_event(&switch_event) != 0U) {
        debug_logln(DEBUG_PRINT_DEBUG, "dbg event=switch id=%s state=%s level=%u",
                    switch_name(switch_event.input),
                    (switch_event.pressed != 0U) ? "pressed" : "released",
                    (unsigned int)switch_event.level);

        if ((switch_event.input == SWITCH_INPUT_BUTTON) && (switch_event.pressed == 0U)) {
            s_app.settings_ui.button_pressed = 0U;
            if (s_app.settings_ui.long_press_fired != 0U) {
                s_app.settings_ui.long_press_fired = 0U;
                continue;
            }

            if (s_app.settings_ui.mode_active != 0U) {
                continue;
            }

            if (s_app.ui.page_count != 0U) {
                s_app.ui.page_index = (uint8_t)((s_app.ui.page_index + 1U) % s_app.ui.page_count);
            }
            s_app.ui.render_dirty = 1U;
        } else if ((switch_event.input == SWITCH_INPUT_BUTTON) && (switch_event.pressed != 0U)) {
            s_app.settings_ui.button_pressed = 1U;
            s_app.settings_ui.long_press_fired = 0U;
            s_app.settings_ui.button_press_start_ms = now_ms;
        }
    }

    if ((s_app.settings_ui.button_pressed != 0U) &&
        (s_app.settings_ui.long_press_fired == 0U) &&
        (input_has_elapsed_ms(now_ms, s_app.settings_ui.button_press_start_ms, APP_SETTINGS_LONG_PRESS_MS) != 0U)) {
        s_app.settings_ui.long_press_fired = 1U;
        if (s_app.settings_ui.mode_active != 0U) {
            app_exit_settings_mode_discard(now_ms);
        } else {
            app_enter_settings_mode(now_ms);
        }
    }
}

void app_process_encoder_events(uint32_t now_ms)
{
    encoder_event_t encoder_event;

    encoder_input_tick(now_ms);
    while (encoder_input_pop_event(&encoder_event) != 0U) {
        debug_logln(DEBUG_PRINT_DEBUG, "dbg event=encoder type=%s sw_level=%u",
                    encoder_event_name(encoder_event.type),
                    (unsigned int)encoder_event.sw_level);
        app_handle_encoder_event(&encoder_event);
    }
}
