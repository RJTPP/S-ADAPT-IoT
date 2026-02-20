#include "app/app_internal.h"

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
    if ((s_app.platform.display_ready != 0U) &&
        (input_has_elapsed_ms(now_ms, s_app.timing.last_oled_ms, s_timing_cfg.oled_update_ms) != 0U)) {
        s_app.timing.last_oled_ms = now_ms;
        display_show_distance_cm(s_app.sensors.last_valid_distance_cm);
    }
}

void app_log_summary_if_due(uint32_t now_ms)
{
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_log_ms, s_timing_cfg.log_ms) != 0U) {
        s_app.timing.last_log_ms = now_ms;
        debug_logln(DEBUG_PRINT_INFO,
                    "dbg summary ldr_raw=%u ldr_filt=%u ldr_status=%s dist_cm_raw_last_valid=%lu dist_cm_filt=%lu us_status=%s present=%u light_on=%u offset=%ld auto=%u target_out=%u hyst_out=%u applied_out=%u rgb=%s",
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
                    status_led_state_to_string(s_app.control.rgb_state));
    }
}
