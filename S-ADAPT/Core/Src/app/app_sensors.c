#include "app/app_internal.h"

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
            s_app.sensors.last_distance_raw_cm = distance_cm;
            filter_median3_u32_push(&s_app.sensors.dist_median3, distance_cm);
            s_app.sensors.last_distance_filtered_cm = filter_median3_u32_get(&s_app.sensors.dist_median3);
            s_app.sensors.last_valid_distance_cm = s_app.sensors.last_distance_filtered_cm;
            s_app.sensors.last_valid_presence = (s_app.sensors.last_distance_filtered_cm < s_policy_cfg.presence_cm) ? 1U : 0U;
        }
    }
}
