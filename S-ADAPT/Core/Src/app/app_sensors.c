#include "app/app_internal.h"

static uint32_t abs_diff_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

void app_sample_ldr_if_due(uint32_t now_ms)
{
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_ldr_sample_ms, s_timing_cfg.ldr_sample_ms) != 0U) {
        s_app.timing.last_ldr_sample_ms += s_timing_cfg.ldr_sample_ms;
        s_app.sensors.last_ldr_status = ldr_read_raw(&s_app.sensors.last_ldr_raw);
        if (s_app.sensors.last_ldr_status == LDR_STATUS_OK) {
            s_app.sensors.last_ldr_filtered =
                filter_moving_average_u16_push(&s_app.sensors.ldr_ma, s_app.sensors.last_ldr_raw);
        }
    }
}

void app_sample_ultrasonic_if_due(uint32_t now_ms)
{
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_us_sample_ms, s_timing_cfg.us_sample_ms) != 0U) {
        uint32_t distance_cm;

        s_app.timing.last_us_sample_ms += s_timing_cfg.us_sample_ms;
        distance_cm = ultrasonic_read_distance_cm(s_policy_cfg.us_timeout_us, s_policy_cfg.distance_error_cm);
        s_app.sensors.last_us_status = ultrasonic_get_last_status();

        if ((distance_cm != s_policy_cfg.distance_error_cm) && (s_app.sensors.last_us_status == ULTRASONIC_STATUS_OK)) {
            uint32_t ref_distance_cm;
            uint32_t abs_step_delta_cm = 0U;
            uint8_t prev_ready;
            uint8_t away_condition;
            uint8_t flat_condition;
            uint8_t motion_condition;

            s_app.sensors.last_distance_raw_cm = distance_cm;
            filter_median3_u32_push(&s_app.sensors.dist_median3, distance_cm);
            s_app.sensors.last_distance_filtered_cm = filter_median3_u32_get(&s_app.sensors.dist_median3);
            s_app.sensors.last_valid_distance_cm = s_app.sensors.last_distance_filtered_cm;

            if (s_app.sensors.ref_pending_capture != 0U) {
                s_app.sensors.ref_distance_cm = s_app.sensors.last_distance_filtered_cm;
                s_app.sensors.ref_valid = 1U;
                s_app.sensors.ref_pending_capture = 0U;
                s_app.sensors.using_fallback_ref = 0U;
            }

            if (s_app.control.light_enabled == 0U) {
                s_app.sensors.away_streak_ms = 0U;
                s_app.sensors.flat_streak_ms = 0U;
                s_app.sensors.motion_streak_ms = 0U;
                s_app.sensors.near_ref_streak_ms = 0U;
                s_app.sensors.presence_candidate_no_user = 0U;
            } else {
                ref_distance_cm = s_app.sensors.ref_valid != 0U ? s_app.sensors.ref_distance_cm : s_policy_cfg.presence_ref_fallback_cm;
                prev_ready = s_app.sensors.prev_valid_distance_ready;
                if (prev_ready != 0U) {
                    abs_step_delta_cm = abs_diff_u32(s_app.sensors.last_distance_filtered_cm, s_app.sensors.prev_valid_distance_cm);
                }

                away_condition = (s_app.sensors.last_distance_filtered_cm > (ref_distance_cm + s_policy_cfg.presence_body_margin_cm)) ? 1U : 0U;
                flat_condition = ((prev_ready != 0U) && (abs_step_delta_cm <= s_policy_cfg.presence_flat_band_cm)) ? 1U : 0U;
                motion_condition = ((prev_ready != 0U) && (abs_step_delta_cm >= s_policy_cfg.presence_motion_delta_cm)) ? 1U : 0U;

                if (s_app.sensors.last_valid_presence != 0U) {
                    if (away_condition != 0U) {
                        s_app.sensors.away_streak_ms += s_timing_cfg.us_sample_ms;
                    } else {
                        s_app.sensors.away_streak_ms = 0U;
                    }

                    if (flat_condition != 0U) {
                        s_app.sensors.flat_streak_ms += s_timing_cfg.us_sample_ms;
                    } else {
                        s_app.sensors.flat_streak_ms = 0U;
                    }
                } else {
                    s_app.sensors.away_streak_ms = 0U;
                    s_app.sensors.flat_streak_ms = 0U;
                }

                /* Motion streak is only meaningful for recovery from flat no-user state. */
                if ((s_app.sensors.last_valid_presence == 0U) &&
                    (s_app.sensors.no_user_reason == APP_NO_USER_REASON_FLAT)) {
                    if (motion_condition != 0U) {
                        s_app.sensors.motion_streak_ms += s_timing_cfg.us_sample_ms;
                    } else {
                        if (s_app.sensors.motion_streak_ms > (s_timing_cfg.us_sample_ms / 2U)) {
                            s_app.sensors.motion_streak_ms -= (s_timing_cfg.us_sample_ms / 2U);
                        } else {
                            s_app.sensors.motion_streak_ms = 0U;
                        }
                    }
                } else {
                    s_app.sensors.motion_streak_ms = 0U;
                }

                if ((s_app.sensors.last_valid_presence == 0U) &&
                    (s_app.sensors.no_user_reason == APP_NO_USER_REASON_AWAY) &&
                    (s_app.sensors.last_distance_filtered_cm <= (ref_distance_cm + s_policy_cfg.presence_return_band_cm))) {
                    s_app.sensors.near_ref_streak_ms += s_timing_cfg.us_sample_ms;
                } else {
                    s_app.sensors.near_ref_streak_ms = 0U;
                }

                s_app.sensors.presence_candidate_no_user = 0U;
                if (s_app.sensors.last_valid_presence != 0U) {
                    s_app.sensors.no_user_reason = APP_NO_USER_REASON_NONE;
                    if (s_app.sensors.away_streak_ms >= s_policy_cfg.presence_away_timeout_ms) {
                        s_app.sensors.presence_candidate_no_user = 1U;
                        s_app.sensors.no_user_reason = APP_NO_USER_REASON_AWAY;
                    } else if (s_app.sensors.flat_streak_ms >= s_policy_cfg.presence_stale_timeout_ms) {
                        s_app.sensors.presence_candidate_no_user = 1U;
                        s_app.sensors.no_user_reason = APP_NO_USER_REASON_FLAT;
                    }
                } else {
                    if ((s_app.sensors.no_user_reason == APP_NO_USER_REASON_AWAY) &&
                        (s_app.sensors.near_ref_streak_ms >= s_policy_cfg.presence_return_confirm_ms)) {
                        s_app.sensors.last_valid_presence = 1U;
                        s_app.sensors.near_ref_streak_ms = 0U;
                        s_app.sensors.away_streak_ms = 0U;
                        s_app.sensors.flat_streak_ms = 0U;
                        s_app.sensors.no_user_reason = APP_NO_USER_REASON_NONE;
                    } else if ((s_app.sensors.no_user_reason == APP_NO_USER_REASON_FLAT) &&
                               (motion_condition != 0U)) {
                        s_app.sensors.last_valid_presence = 1U;
                        s_app.sensors.motion_streak_ms = 0U;
                        s_app.sensors.away_streak_ms = 0U;
                        s_app.sensors.flat_streak_ms = 0U;
                        s_app.sensors.no_user_reason = APP_NO_USER_REASON_NONE;
                    }
                }
            }

            s_app.sensors.prev_valid_distance_cm = s_app.sensors.last_distance_filtered_cm;
            s_app.sensors.prev_valid_distance_ready = 1U;
        }
    }
}
