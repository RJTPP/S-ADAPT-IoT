#include "app/app_internal.h"

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

void app_handle_click_timeout(uint32_t now_ms)
{
    uint8_t was_light_enabled;

    if (s_app.click.pending == 0U) {
        return;
    }

    if (input_has_elapsed_ms(now_ms, s_app.click.deadline_ms, s_policy_cfg.double_click_ms) == 0U) {
        return;
    }

    s_app.click.pending = 0U;
    was_light_enabled = s_app.control.light_enabled;
    s_app.control.light_enabled = (s_app.control.light_enabled == 0U) ? 1U : 0U;

    if ((was_light_enabled == 0U) && (s_app.control.light_enabled != 0U)) {
        s_app.sensors.ref_distance_cm = s_policy_cfg.presence_ref_fallback_cm;
        s_app.sensors.ref_valid = 1U;
        s_app.sensors.ref_pending_capture = 1U;
        s_app.sensors.using_fallback_ref = 1U;
        s_app.sensors.prev_valid_distance_ready = 0U;
        s_app.sensors.away_streak_ms = 0U;
        s_app.sensors.flat_streak_ms = 0U;
        s_app.sensors.motion_streak_ms = 0U;
        s_app.sensors.no_user_reason = 0U;
        s_app.sensors.presence_candidate_no_user = 0U;
        s_app.sensors.last_valid_presence = 1U;
        s_app.control.preoff_active = 0U;
        s_app.control.preoff_start_ms = 0U;
        s_app.control.preoff_dim_target_percent = 0U;
    } else if ((was_light_enabled != 0U) && (s_app.control.light_enabled == 0U)) {
        s_app.sensors.away_streak_ms = 0U;
        s_app.sensors.flat_streak_ms = 0U;
        s_app.sensors.motion_streak_ms = 0U;
        s_app.sensors.no_user_reason = 0U;
        s_app.sensors.presence_candidate_no_user = 0U;
        s_app.control.preoff_active = 0U;
        s_app.control.preoff_start_ms = 0U;
        s_app.control.preoff_dim_target_percent = 0U;
    }

    debug_logln(DEBUG_PRINT_INFO, "dbg click=single light_on=%u", (unsigned int)s_app.control.light_enabled);
}

void app_handle_encoder_event(const encoder_event_t *event)
{
    if (event == NULL) {
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
            if ((s_app.click.pending != 0U) &&
                ((uint32_t)(event->timestamp_ms - s_app.click.deadline_ms) < s_policy_cfg.double_click_ms)) {
                s_app.control.manual_offset = 0;
                s_app.click.pending = 0U;
                debug_logln(DEBUG_PRINT_INFO, "dbg click=double offset=%ld", (long)s_app.control.manual_offset);
            } else {
                s_app.click.pending = 1U;
                s_app.click.deadline_ms = event->timestamp_ms;
            }
            break;

        default:
            break;
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
