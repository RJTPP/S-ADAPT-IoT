#include "app/app.h"

#include "support/debug_print.h"
#include "bsp/display.h"
#include "bsp/main_led.h"
#include "bsp/status_led.h"
#include "input/encoder_input.h"
#include "input/switch_input.h"
#include "sensors/ldr.h"
#include "sensors/ultrasonic.h"

#define APP_CONTROL_TICK_MS   50U
#define APP_US_SAMPLE_MS      100U
#define APP_LOG_MS            1000U
#define APP_OLED_UPDATE_MS    1000U
#define APP_BOOT_SETUP_MS     1000U
#define APP_PRESENCE_CM       80U
#define APP_DOUBLE_CLICK_MS   350U
#define APP_OFFSET_STEP       5
#define APP_OFFSET_MIN        (-50)
#define APP_OFFSET_MAX        50
#define APP_DISTANCE_ERROR_CM 999U
#define APP_US_TIMEOUT_US     30000U

#ifndef APP_ENABLE_DISPLAY
#define APP_ENABLE_DISPLAY    1U
#endif

typedef struct
{
    uint32_t boot_start_ms;
    uint32_t last_control_tick_ms;
    uint32_t last_us_sample_ms;
    uint32_t last_log_ms;
    uint32_t last_oled_ms;

    uint16_t last_ldr_raw;
    ldr_status_t last_ldr_status;
    uint32_t last_valid_distance_cm;
    ultrasonic_status_t last_us_status;
    uint8_t last_valid_presence;

    uint8_t light_enabled;
    int32_t manual_offset;
    uint8_t auto_percent;
    uint8_t output_percent;

    uint8_t click_pending;
    uint32_t click_deadline_ms;
    uint32_t last_press_ms;
    uint32_t last_release_ms;

    uint8_t fatal_fault;
    uint8_t display_ready;

    status_led_state_t rgb_state;
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
    if (offset < APP_OFFSET_MIN) {
        return APP_OFFSET_MIN;
    }
    if (offset > APP_OFFSET_MAX) {
        return APP_OFFSET_MAX;
    }

    return offset;
}

static uint8_t compute_auto_percent_from_ldr(uint16_t raw)
{
    uint32_t scaled = ((uint32_t)raw * 100U) / 4095U;

    if (scaled > 100U) {
        scaled = 100U;
    }

    return (uint8_t)(100U - scaled);
}

static status_led_state_t app_evaluate_state(uint32_t now_ms)
{
    if (s_app.fatal_fault != 0U) {
        return STATUS_LED_STATE_FAULT_FATAL;
    }

    if ((uint32_t)(now_ms - s_app.boot_start_ms) < APP_BOOT_SETUP_MS) {
        return STATUS_LED_STATE_BOOT_SETUP;
    }

    if (s_app.light_enabled == 0U) {
        return STATUS_LED_STATE_LIGHT_OFF;
    }

    if (s_app.last_valid_presence == 0U) {
        return STATUS_LED_STATE_NO_USER;
    }

    if (s_app.manual_offset != 0) {
        return STATUS_LED_STATE_OFFSET_POSITIVE;
    }

    return STATUS_LED_STATE_AUTO;
}

static void app_handle_click_timeout(uint32_t now_ms)
{
    if ((s_app.click_pending == 0U) || ((int32_t)(now_ms - s_app.click_deadline_ms) < 0)) {
        return;
    }

    s_app.click_pending = 0U;
    s_app.light_enabled = (s_app.light_enabled == 0U) ? 1U : 0U;
    debug_logln(DEBUG_PRINT_INFO, "dbg click=single light_on=%u", (unsigned int)s_app.light_enabled);
}

static void app_handle_encoder_event(const encoder_event_t *event)
{
    if (event == NULL) {
        return;
    }

    switch (event->type) {
        case ENCODER_EVENT_CW:
            if (s_app.light_enabled != 0U) {
                s_app.manual_offset = clamp_offset(s_app.manual_offset + APP_OFFSET_STEP);
            }
            break;

        case ENCODER_EVENT_CCW:
            if (s_app.light_enabled != 0U) {
                s_app.manual_offset = clamp_offset(s_app.manual_offset - APP_OFFSET_STEP);
            }
            break;

        case ENCODER_EVENT_SW_PRESSED:
            s_app.last_press_ms = event->timestamp_ms;
            break;

        case ENCODER_EVENT_SW_RELEASED:
            s_app.last_release_ms = event->timestamp_ms;
            if ((s_app.click_pending != 0U) &&
                ((int32_t)(event->timestamp_ms - s_app.click_deadline_ms) <= 0)) {
                s_app.manual_offset = 0;
                s_app.click_pending = 0U;
                debug_logln(DEBUG_PRINT_INFO, "dbg click=double offset=%ld", (long)s_app.manual_offset);
            } else {
                s_app.click_pending = 1U;
                s_app.click_deadline_ms = event->timestamp_ms + APP_DOUBLE_CLICK_MS;
            }
            break;

        default:
            break;
    }
}

void app_set_fatal_fault(uint8_t enabled)
{
    s_app.fatal_fault = (enabled != 0U) ? 1U : 0U;
    status_led_set_fatal_fault(s_app.fatal_fault);
}

uint8_t app_init(ADC_HandleTypeDef *ldr_adc,
                 TIM_HandleTypeDef *echo_tim,
                 uint32_t echo_channel,
                 TIM_HandleTypeDef *main_led_tim,
                 uint32_t main_led_channel)
{
    uint32_t now_ms = HAL_GetTick();
    uint8_t ok = 1U;
    main_led_status_t main_led_status;

    s_app.boot_start_ms = now_ms;
    s_app.last_control_tick_ms = now_ms;
    s_app.last_us_sample_ms = now_ms;
    s_app.last_log_ms = now_ms;
    s_app.last_oled_ms = now_ms;

    s_app.last_ldr_raw = 0U;
    s_app.last_ldr_status = LDR_STATUS_NOT_INIT;
    s_app.last_valid_distance_cm = APP_DISTANCE_ERROR_CM;
    s_app.last_us_status = ULTRASONIC_STATUS_NOT_INIT;
    s_app.last_valid_presence = 1U;

    s_app.light_enabled = 0U;
    s_app.manual_offset = 0;
    s_app.auto_percent = 0U;
    s_app.output_percent = 0U;

    s_app.click_pending = 0U;
    s_app.click_deadline_ms = now_ms;
    s_app.last_press_ms = now_ms;
    s_app.last_release_ms = now_ms;

    s_app.fatal_fault = 0U;
    s_app.display_ready = 0U;
    s_app.rgb_state = STATUS_LED_STATE_BOOT_SETUP;

    ldr_init(ldr_adc);
    ultrasonic_init(echo_tim, echo_channel);
    switch_input_init();
    encoder_input_init();
    status_led_init();

    main_led_init(main_led_tim, main_led_channel);
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

#if APP_ENABLE_DISPLAY
    if (display_init() != 0U) {
        s_app.display_ready = 1U;
        display_show_boot();
        debug_logln(DEBUG_PRINT_INFO, "dbg oled=ready");
    } else {
        s_app.display_ready = 0U;
        debug_logln(DEBUG_PRINT_ERROR, "dbg oled=init_failed");
    }
#else
    s_app.display_ready = 0U;
    debug_logln(DEBUG_PRINT_INFO, "dbg oled=disabled");
#endif

    s_app.rgb_state = app_evaluate_state(now_ms);
    status_led_set_state(s_app.rgb_state);
    debug_logln(DEBUG_PRINT_INFO, "dbg rgb_state=%s", status_led_state_to_string(s_app.rgb_state));

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

    if ((uint32_t)(now_ms - s_app.last_control_tick_ms) < APP_CONTROL_TICK_MS) {
        return;
    }
    s_app.last_control_tick_ms = now_ms;

    s_app.last_ldr_status = ldr_read_raw(&s_app.last_ldr_raw);

    if ((uint32_t)(now_ms - s_app.last_us_sample_ms) >= APP_US_SAMPLE_MS) {
        uint32_t distance_cm;

        s_app.last_us_sample_ms = now_ms;
        distance_cm = ultrasonic_read_distance_cm(APP_US_TIMEOUT_US, APP_DISTANCE_ERROR_CM);
        s_app.last_us_status = ultrasonic_get_last_status();

        if ((distance_cm != APP_DISTANCE_ERROR_CM) && (s_app.last_us_status == ULTRASONIC_STATUS_OK)) {
            s_app.last_valid_distance_cm = distance_cm;
            s_app.last_valid_presence = (distance_cm < APP_PRESENCE_CM) ? 1U : 0U;
        }
    }

    s_app.auto_percent = compute_auto_percent_from_ldr(s_app.last_ldr_raw);

    target_percent_i32 = (int32_t)s_app.auto_percent + s_app.manual_offset;
    s_app.output_percent = clamp_percent_i32(target_percent_i32);

    if ((s_app.light_enabled == 0U) || (s_app.last_valid_presence == 0U)) {
        s_app.output_percent = 0U;
    }

    (void)main_led_set_percent(s_app.output_percent);

    s_app.rgb_state = app_evaluate_state(now_ms);
    status_led_set_state(s_app.rgb_state);
    status_led_tick(now_ms);

    if ((s_app.display_ready != 0U) && ((uint32_t)(now_ms - s_app.last_oled_ms) >= APP_OLED_UPDATE_MS)) {
        s_app.last_oled_ms = now_ms;
        display_show_distance_cm(s_app.last_valid_distance_cm);
    }

    if ((uint32_t)(now_ms - s_app.last_log_ms) >= APP_LOG_MS) {
        s_app.last_log_ms = now_ms;
        debug_logln(DEBUG_PRINT_INFO,
                    "dbg summary ldr_raw=%u ldr_status=%s dist_cm=%lu us_status=%s present=%u light_on=%u offset=%ld auto=%u out=%u rgb=%s",
                    (unsigned int)s_app.last_ldr_raw,
                    ldr_status_to_string(s_app.last_ldr_status),
                    (unsigned long)s_app.last_valid_distance_cm,
                    ultrasonic_status_to_string(s_app.last_us_status),
                    (unsigned int)s_app.last_valid_presence,
                    (unsigned int)s_app.light_enabled,
                    (long)s_app.manual_offset,
                    (unsigned int)s_app.auto_percent,
                    (unsigned int)s_app.output_percent,
                    status_led_state_to_string(s_app.rgb_state));
    }
}
