#include "app/app.h"

#include "support/debug_print.h"
#include "support/filter_utils.h"
#include "bsp/display.h"
#include "bsp/main_led.h"
#include "bsp/status_led.h"
#include "input/encoder_input.h"
#include "input/switch_input.h"
#include "sensors/ldr.h"
#include "sensors/ultrasonic.h"

typedef struct
{
    uint32_t control_tick_ms;
    uint32_t us_sample_ms;
    uint32_t log_ms;
    uint32_t oled_update_ms;
} app_timing_cfg_t;

typedef struct
{
    uint32_t boot_setup_ms;
    uint32_t presence_cm;
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
} app_policy_cfg_t;

#ifndef APP_ENABLE_DISPLAY
#define APP_ENABLE_DISPLAY 1U
#endif

static const app_timing_cfg_t s_timing_cfg = {
    .control_tick_ms = 50U,
    .us_sample_ms = 100U,
    .log_ms = 1000U,
    .oled_update_ms = 1000U,
};

static const app_policy_cfg_t s_policy_cfg = {
    .boot_setup_ms = 1000U,
    .presence_cm = 80U,
    .double_click_ms = 350U,
    .offset_step = 5,
    .offset_min = -50,
    .offset_max = 50,
    .distance_error_cm = 999U,
    .us_timeout_us = 30000U,
    .ldr_ma_window_size = 8U,
    .output_hysteresis_band_percent = 5U,
    .output_ramp_step_percent = 2U,
    .output_ramp_step_on_percent = 5U,
    .output_ramp_step_off_percent = 8U,
};

typedef struct
{
    uint32_t boot_start_ms;
    uint32_t last_control_tick_ms;
    uint32_t last_us_sample_ms;
    uint32_t last_log_ms;
    uint32_t last_oled_ms;
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
    uint8_t display_ready;
} app_platform_state_t;

typedef struct
{
    app_timing_state_t timing;
    app_sensor_state_t sensors;
    app_control_state_t control;
    app_click_state_t click;
    app_platform_state_t platform;
} app_ctx_t;

static app_ctx_t s_app;

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

static const char *status_led_state_to_string(status_led_state_t state)
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

static status_led_state_t app_evaluate_state(uint32_t now_ms)
{
    if (s_app.control.fatal_fault != 0U) {
        return STATUS_LED_STATE_FAULT_FATAL;
    }

    if ((uint32_t)(now_ms - s_app.timing.boot_start_ms) < s_policy_cfg.boot_setup_ms) {
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

static void app_handle_click_timeout(uint32_t now_ms)
{
    if (s_app.click.pending == 0U) {
        return;
    }

    if ((uint32_t)(now_ms - s_app.click.deadline_ms) < s_policy_cfg.double_click_ms) {
        return;
    }

    s_app.click.pending = 0U;
    s_app.control.light_enabled = (s_app.control.light_enabled == 0U) ? 1U : 0U;
    debug_logln(DEBUG_PRINT_INFO, "dbg click=single light_on=%u", (unsigned int)s_app.control.light_enabled);
}

static void app_handle_encoder_event(const encoder_event_t *event)
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
    s_app.timing.last_us_sample_ms = now_ms;
    s_app.timing.last_log_ms = now_ms;
    s_app.timing.last_oled_ms = now_ms;

    s_app.sensors.last_ldr_raw = 0U;
    s_app.sensors.last_ldr_filtered = 0U;
    s_app.sensors.last_ldr_status = LDR_STATUS_NOT_INIT;

    s_app.sensors.last_distance_raw_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_distance_filtered_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_valid_distance_cm = s_policy_cfg.distance_error_cm;
    s_app.sensors.last_us_status = ULTRASONIC_STATUS_NOT_INIT;
    s_app.sensors.last_valid_presence = 1U;

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
    s_app.control.fatal_fault = 0U;
    s_app.control.rgb_state = STATUS_LED_STATE_BOOT_SETUP;

    s_app.click.pending = 0U;
    s_app.click.deadline_ms = now_ms;
    s_app.click.last_press_ms = now_ms;
    s_app.click.last_release_ms = now_ms;

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
    switch_input_event_t switch_event;
    encoder_event_t encoder_event;
    int32_t target_percent_i32;

    status_led_tick(now_ms);

    switch_input_tick(now_ms);
    while (switch_input_pop_event(&switch_event) != 0U) {
        debug_logln(DEBUG_PRINT_DEBUG, "dbg event=switch id=%s state=%s level=%u",
                    switch_name(switch_event.input),
                    (switch_event.pressed != 0U) ? "pressed" : "released",
                    (unsigned int)switch_event.level);
    }

    encoder_input_tick(now_ms);
    while (encoder_input_pop_event(&encoder_event) != 0U) {
        debug_logln(DEBUG_PRINT_DEBUG, "dbg event=encoder type=%s sw_level=%u",
                    encoder_event_name(encoder_event.type),
                    (unsigned int)encoder_event.sw_level);
        app_handle_encoder_event(&encoder_event);
    }

    app_handle_click_timeout(now_ms);

    if ((uint32_t)(now_ms - s_app.timing.last_control_tick_ms) < s_timing_cfg.control_tick_ms) {
        return;
    }
    s_app.timing.last_control_tick_ms = now_ms;

    s_app.sensors.last_ldr_status = ldr_read_raw(&s_app.sensors.last_ldr_raw);
    if (s_app.sensors.last_ldr_status == LDR_STATUS_OK) {
        s_app.sensors.last_ldr_filtered = filter_moving_average_u16_push(&s_app.sensors.ldr_ma, s_app.sensors.last_ldr_raw);
    }

    if ((uint32_t)(now_ms - s_app.timing.last_us_sample_ms) >= s_timing_cfg.us_sample_ms) {
        uint32_t distance_cm;

        s_app.timing.last_us_sample_ms = now_ms;
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

    s_app.control.rgb_state = app_evaluate_state(now_ms);
    status_led_set_state(s_app.control.rgb_state);
    status_led_tick(now_ms);

    if ((s_app.platform.display_ready != 0U) &&
        ((uint32_t)(now_ms - s_app.timing.last_oled_ms) >= s_timing_cfg.oled_update_ms)) {
        s_app.timing.last_oled_ms = now_ms;
        display_show_distance_cm(s_app.sensors.last_valid_distance_cm);
    }

    if ((uint32_t)(now_ms - s_app.timing.last_log_ms) >= s_timing_cfg.log_ms) {
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
