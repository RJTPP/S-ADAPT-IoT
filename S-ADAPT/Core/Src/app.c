#include "app.h"

#include "debug_print.h"
#include "display.h"
#include "status_led.h"
#include "ultrasonic.h"

#define APP_ECHO_TIMEOUT_US    30000U
#define APP_DISTANCE_ERROR_CM  999U
#define APP_LOOP_TICK_MS       100U
#define APP_BOOT_SETUP_MS      1000U
#define APP_PRESENCE_CM        80U
#define APP_RGB_TEST_MODE      1
#define APP_TEST_STATE_MS      1000U

typedef struct
{
    uint32_t boot_start_ms;
    uint32_t last_tick_ms;
    uint32_t last_valid_distance_cm;
    int32_t manual_offset;
    uint8_t last_valid_presence;
    uint8_t light_enabled;
    uint8_t fatal_fault;
    uint8_t display_ready;
    uint8_t has_logged_state;
    status_led_state_t last_logged_state;
} app_ctx_t;

static app_ctx_t s_app;

static const char *status_led_state_to_string(status_led_state_t state)
{
    switch (state) {
        case STATUS_LED_STATE_BOOT_SETUP:
            return "boot_setup";
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

static status_led_state_t app_evaluate_state(uint32_t now_ms)
{
    if (s_app.fatal_fault != 0U) {
        return STATUS_LED_STATE_FAULT_FATAL;
    }

    if ((now_ms - s_app.boot_start_ms) < APP_BOOT_SETUP_MS) {
        return STATUS_LED_STATE_BOOT_SETUP;
    }

    if ((s_app.light_enabled != 0U) && (s_app.last_valid_presence == 0U)) {
        return STATUS_LED_STATE_NO_USER;
    }

    if ((s_app.light_enabled != 0U) && (s_app.manual_offset > 0)) {
        return STATUS_LED_STATE_OFFSET_POSITIVE;
    }

    return STATUS_LED_STATE_AUTO;
}

static status_led_state_t app_apply_test_override(status_led_state_t runtime_state, uint32_t now_ms)
{
#if APP_RGB_TEST_MODE
    uint32_t phase;

    if (runtime_state == STATUS_LED_STATE_FAULT_FATAL) {
        return runtime_state;
    }

    phase = ((now_ms - s_app.boot_start_ms) / APP_TEST_STATE_MS) % 4U;
    switch (phase) {
        case 0U:
            return STATUS_LED_STATE_AUTO;
        case 1U:
            return STATUS_LED_STATE_OFFSET_POSITIVE;
        case 2U:
            return STATUS_LED_STATE_NO_USER;
        default:
            return STATUS_LED_STATE_BOOT_SETUP;
    }
#else
    (void)now_ms;
    return runtime_state;
#endif
}

void app_set_fatal_fault(uint8_t enabled)
{
    s_app.fatal_fault = (enabled != 0U) ? 1U : 0U;
    status_led_set_fatal_fault(s_app.fatal_fault);
}

uint8_t app_init(TIM_HandleTypeDef *echo_tim, uint32_t echo_channel)
{
    uint32_t now_ms = HAL_GetTick();
    uint8_t ok = 1U;

    s_app.boot_start_ms = now_ms;
    s_app.last_tick_ms = now_ms;
    s_app.last_valid_distance_cm = APP_DISTANCE_ERROR_CM;
    s_app.manual_offset = 0;
    s_app.last_valid_presence = 1U;
    s_app.light_enabled = 0U;
    s_app.fatal_fault = 0U;
    s_app.display_ready = 0U;
    s_app.has_logged_state = 0U;
    s_app.last_logged_state = STATUS_LED_STATE_AUTO;

    status_led_init();
    ultrasonic_init(echo_tim, echo_channel);

    if (!display_init()) {
        ok = 0U;
        app_set_fatal_fault(1U);
        debug_logln(DEBUG_PRINT_ERROR, "display init failed, fatal fault enabled");
    } else {
        s_app.display_ready = 1U;
        display_show_boot();
    }

    s_app.boot_start_ms = HAL_GetTick();
    s_app.last_tick_ms = s_app.boot_start_ms;

    return ok;
}

void app_step(void)
{
    uint32_t now_ms = HAL_GetTick();
    status_led_state_t state;

    status_led_tick(now_ms);

    if ((now_ms - s_app.last_tick_ms) < APP_LOOP_TICK_MS) {
        return;
    }
    s_app.last_tick_ms = now_ms;

    {
        uint32_t distance_cm = ultrasonic_read_distance_cm(APP_ECHO_TIMEOUT_US, APP_DISTANCE_ERROR_CM);
        ultrasonic_status_t us_status = ultrasonic_get_last_status();

        if ((distance_cm != APP_DISTANCE_ERROR_CM) && (us_status == ULTRASONIC_STATUS_OK)) {
            s_app.last_valid_distance_cm = distance_cm;
            s_app.last_valid_presence = (distance_cm < APP_PRESENCE_CM) ? 1U : 0U;
        }
    }

    if (s_app.display_ready != 0U) {
        display_show_distance_cm(s_app.last_valid_distance_cm);
    }

    state = app_evaluate_state(now_ms);
    state = app_apply_test_override(state, now_ms);

    status_led_set_state(state);
    status_led_tick(now_ms);

    if ((s_app.has_logged_state == 0U) || (state != s_app.last_logged_state)) {
        debug_logln(DEBUG_PRINT_INFO, "rgb_state=%s", status_led_state_to_string(state));
        s_app.last_logged_state = state;
        s_app.has_logged_state = 1U;
    }
}
