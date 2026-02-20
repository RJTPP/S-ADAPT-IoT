#include "bsp/status_led.h"

#include "main.h"

#define DISTANCE_THRESHOLD_NEAR_CM 10U
#define DISTANCE_THRESHOLD_MID_CM  20U
#define STATUS_LED_FAULT_BLINK_DEFAULT_MS 250U

static status_led_state_t s_state = STATUS_LED_STATE_BOOT_SETUP;
static uint8_t s_fatal_fault_enabled = 0U;
static uint8_t s_fault_red_on = 0U;
static uint32_t s_fault_blink_period_ms = STATUS_LED_FAULT_BLINK_DEFAULT_MS;
static uint32_t s_last_fault_toggle_ms = 0U;

static GPIO_PinState logical_to_pin_state(uint8_t logical_on)
{
#if STATUS_LED_ACTIVE_LOW
    return (logical_on != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
#else
    return (logical_on != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
#endif
}

static void write_channel(GPIO_TypeDef *port, uint16_t pin, uint8_t logical_on)
{
    HAL_GPIO_WritePin(port, pin, logical_to_pin_state(logical_on));
}

static void apply_color(uint8_t r_on, uint8_t g_on, uint8_t b_on)
{
    write_channel(LED_Status_R_GPIO_Port, LED_Status_R_Pin, r_on);
    write_channel(LED_Status_G_GPIO_Port, LED_Status_G_Pin, g_on);
    write_channel(LED_Status_B_GPIO_Port, LED_Status_B_Pin, b_on);
}

static void apply_state_color(status_led_state_t state)
{
    switch (state) {
        case STATUS_LED_STATE_BOOT_SETUP:
            apply_color(1U, 0U, 1U); /* Purple */
            break;
        case STATUS_LED_STATE_LIGHT_OFF:
            apply_color(1U, 0U, 0U); /* Red */
            break;
        case STATUS_LED_STATE_AUTO:
            apply_color(0U, 0U, 1U); /* Blue */
            break;
        case STATUS_LED_STATE_OFFSET_POSITIVE:
            apply_color(0U, 1U, 0U); /* Green */
            break;
        case STATUS_LED_STATE_NO_USER:
            apply_color(1U, 1U, 0U); /* Yellow */
            break;
        case STATUS_LED_STATE_FAULT_FATAL:
            /* Fault state is handled by blink logic. */
            break;
        default:
            apply_color(0U, 0U, 1U);
            break;
    }
}

void status_led_init(void)
{
    s_state = STATUS_LED_STATE_BOOT_SETUP;
    s_fatal_fault_enabled = 0U;
    s_fault_red_on = 0U;
    s_fault_blink_period_ms = STATUS_LED_FAULT_BLINK_DEFAULT_MS;
    s_last_fault_toggle_ms = HAL_GetTick();
    apply_state_color(s_state);
}

void status_led_set_state(status_led_state_t state)
{
    s_state = state;

    if (state == STATUS_LED_STATE_FAULT_FATAL) {
        status_led_set_fatal_fault(1U);
        return;
    }

    if (s_fatal_fault_enabled == 0U) {
        apply_state_color(state);
    }
}

void status_led_tick(uint32_t now_ms)
{
    if ((s_fatal_fault_enabled == 0U) && (s_state != STATUS_LED_STATE_FAULT_FATAL)) {
        return;
    }

    if ((now_ms - s_last_fault_toggle_ms) < s_fault_blink_period_ms) {
        return;
    }

    s_last_fault_toggle_ms = now_ms;
    s_fault_red_on ^= 1U;
    apply_color(s_fault_red_on, 0U, 0U);
}

void status_led_set_fatal_fault(uint8_t enabled)
{
    if (enabled != 0U) {
        s_fatal_fault_enabled = 1U;
        s_fault_red_on = 1U;
        s_last_fault_toggle_ms = HAL_GetTick();
        apply_color(1U, 0U, 0U);
        return;
    }

    s_fatal_fault_enabled = 0U;
    s_fault_red_on = 0U;

    if (s_state == STATUS_LED_STATE_FAULT_FATAL) {
        s_state = STATUS_LED_STATE_AUTO;
    }
    apply_state_color(s_state);
}

void status_led_set_for_distance(uint32_t distance_cm)
{
    /* Deprecated compatibility mapping for legacy distance-based callers. */
    if (distance_cm < DISTANCE_THRESHOLD_NEAR_CM)
    {
        status_led_set_state(STATUS_LED_STATE_NO_USER);
    }
    else if (distance_cm < DISTANCE_THRESHOLD_MID_CM)
    {
        status_led_set_state(STATUS_LED_STATE_OFFSET_POSITIVE);
    }
    else
    {
        status_led_set_state(STATUS_LED_STATE_AUTO);
    }
}

void status_led_blink_error(uint32_t period_ms)
{
    if (period_ms != 0U) {
        s_fault_blink_period_ms = period_ms;
    }

    status_led_set_fatal_fault(1U);
    status_led_tick(HAL_GetTick());
}
