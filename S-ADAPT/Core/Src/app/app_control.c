#include "app/app_internal.h"

static uint8_t clamp_percent_i32(int32_t value)
{
    if (value <= 0) {
        return 0U;
    }
    if (value >= 100) {
        return 100U;
    }

    return (uint8_t)value;
}

static uint8_t compute_auto_percent_from_ldr(uint16_t filtered_raw)
{
    uint32_t scaled = ((uint32_t)filtered_raw * 100U) / 4095U;

    if (scaled > 100U) {
        scaled = 100U;
    }

    return (uint8_t)(100U - scaled);
}

static uint8_t apply_output_hysteresis(uint8_t target_percent)
{
    uint8_t diff;

    if (s_app.control.output_hysteresis_initialized == 0U) {
        s_app.control.last_applied_output_percent = target_percent;
        s_app.control.output_hysteresis_initialized = 1U;
        return target_percent;
    }

    if (target_percent > s_app.control.last_applied_output_percent) {
        diff = (uint8_t)(target_percent - s_app.control.last_applied_output_percent);
    } else {
        diff = (uint8_t)(s_app.control.last_applied_output_percent - target_percent);
    }

    if ((target_percent == 0U) || (diff >= s_policy_cfg.output_hysteresis_band_percent)) {
        s_app.control.last_applied_output_percent = target_percent;
    }

    return s_app.control.last_applied_output_percent;
}

static uint8_t apply_output_ramp(uint8_t desired_percent)
{
    uint8_t current;
    uint8_t step;

    if (s_app.control.ramp_initialized == 0U) {
        s_app.control.ramped_output_percent = desired_percent;
        s_app.control.ramp_initialized = 1U;
        return desired_percent;
    }

    current = s_app.control.ramped_output_percent;
    step = s_policy_cfg.output_ramp_step_percent;
    if ((current == 0U) && (desired_percent > 0U)) {
        step = s_policy_cfg.output_ramp_step_on_percent;
    } else if ((desired_percent == 0U) && (current > 0U)) {
        step = s_policy_cfg.output_ramp_step_off_percent;
    }
    if (step == 0U) {
        step = 1U;
    }

    if (desired_percent > current) {
        uint8_t delta = (uint8_t)(desired_percent - current);
        if (delta > step) {
            current = (uint8_t)(current + step);
        } else {
            current = desired_percent;
        }
    } else if (desired_percent < current) {
        uint8_t delta = (uint8_t)(current - desired_percent);
        if (delta > step) {
            current = (uint8_t)(current - step);
        } else {
            current = desired_percent;
        }
    }

    s_app.control.ramped_output_percent = current;
    return current;
}

status_led_state_t app_evaluate_state(uint32_t now_ms)
{
    if (s_app.control.fatal_fault != 0U) {
        return STATUS_LED_STATE_FAULT_FATAL;
    }

    if (input_has_elapsed_ms(now_ms, s_app.timing.boot_start_ms, s_policy_cfg.boot_setup_ms) == 0U) {
        return STATUS_LED_STATE_BOOT_SETUP;
    }

    if (s_app.control.light_enabled == 0U) {
        return STATUS_LED_STATE_LIGHT_OFF;
    }

    if (s_app.sensors.last_valid_presence == 0U) {
        return STATUS_LED_STATE_NO_USER;
    }

    if (s_app.control.manual_offset != 0) {
        return STATUS_LED_STATE_OFFSET_POSITIVE;
    }

    return STATUS_LED_STATE_AUTO;
}

uint8_t app_control_tick_due(uint32_t now_ms)
{
    if (input_has_elapsed_ms(now_ms, s_app.timing.last_control_tick_ms, s_timing_cfg.control_tick_ms) == 0U) {
        return 0U;
    }

    s_app.timing.last_control_tick_ms += s_timing_cfg.control_tick_ms;
    return 1U;
}

void app_update_output_control(void)
{
    int32_t target_percent_i32;

    s_app.control.auto_percent = compute_auto_percent_from_ldr(s_app.sensors.last_ldr_filtered);

    target_percent_i32 = (int32_t)s_app.control.auto_percent + s_app.control.manual_offset;
    s_app.control.target_output_percent = clamp_percent_i32(target_percent_i32);

    if ((s_app.control.light_enabled == 0U) || (s_app.sensors.last_valid_presence == 0U)) {
        s_app.control.target_output_percent = 0U;
    }

    s_app.control.hysteresis_output_percent = apply_output_hysteresis(s_app.control.target_output_percent);
    s_app.control.ramped_output_percent = apply_output_ramp(s_app.control.hysteresis_output_percent);
    s_app.control.output_percent = s_app.control.ramped_output_percent;
    (void)main_led_set_percent(s_app.control.output_percent);
}

void app_update_rgb(uint32_t now_ms)
{
    s_app.control.rgb_state = app_evaluate_state(now_ms);
    status_led_set_state(s_app.control.rgb_state);
    status_led_tick(now_ms);
}
