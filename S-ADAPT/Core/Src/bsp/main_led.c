#include "bsp/main_led.h"

static TIM_HandleTypeDef *s_main_led_tim = NULL;
static uint32_t s_main_led_channel = 0U;
static uint8_t s_main_led_started = 0U;
static uint8_t s_main_led_enabled = 0U;
static uint8_t s_main_led_percent = 0U;

static main_led_status_t apply_percent(uint8_t percent)
{
    uint32_t arr;
    uint32_t pulse;

    if (s_main_led_tim == NULL) {
        return MAIN_LED_STATUS_NOT_INIT;
    }

    arr = __HAL_TIM_GET_AUTORELOAD(s_main_led_tim);
    pulse = (((arr + 1U) * (uint32_t)percent) / 100U);
    if (pulse > arr) {
        pulse = arr;
    }

    __HAL_TIM_SET_COMPARE(s_main_led_tim, s_main_led_channel, pulse);
    return MAIN_LED_STATUS_OK;
}

void main_led_init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    s_main_led_tim = htim;
    s_main_led_channel = channel;
    s_main_led_started = 0U;
    s_main_led_enabled = 0U;
    s_main_led_percent = 0U;
}

main_led_status_t main_led_start(void)
{
    if (s_main_led_tim == NULL) {
        return MAIN_LED_STATUS_NOT_INIT;
    }

    if (HAL_TIM_PWM_Start(s_main_led_tim, s_main_led_channel) != HAL_OK) {
        return MAIN_LED_STATUS_HAL_START_ERROR;
    }

    s_main_led_started = 1U;
    return apply_percent(0U);
}

main_led_status_t main_led_set_percent(uint8_t percent)
{
    if (percent > 100U) {
        return MAIN_LED_STATUS_INVALID_PERCENT;
    }
    if (s_main_led_tim == NULL) {
        return MAIN_LED_STATUS_NOT_INIT;
    }
    if (s_main_led_started == 0U) {
        return MAIN_LED_STATUS_NOT_INIT;
    }

    s_main_led_percent = percent;
    if (s_main_led_enabled == 0U) {
        return apply_percent(0U);
    }

    return apply_percent(s_main_led_percent);
}

uint8_t main_led_get_percent(void)
{
    return s_main_led_percent;
}

main_led_status_t main_led_set_enabled(uint8_t enabled)
{
    if (s_main_led_tim == NULL) {
        return MAIN_LED_STATUS_NOT_INIT;
    }
    if (s_main_led_started == 0U) {
        return MAIN_LED_STATUS_NOT_INIT;
    }

    s_main_led_enabled = (enabled != 0U) ? 1U : 0U;
    if (s_main_led_enabled == 0U) {
        return apply_percent(0U);
    }

    return apply_percent(s_main_led_percent);
}

const char *main_led_status_to_string(main_led_status_t status)
{
    switch (status) {
        case MAIN_LED_STATUS_OK:
            return "ok";
        case MAIN_LED_STATUS_NOT_INIT:
            return "not_init";
        case MAIN_LED_STATUS_NULL_PTR:
            return "null_ptr";
        case MAIN_LED_STATUS_INVALID_PERCENT:
            return "invalid_percent";
        case MAIN_LED_STATUS_HAL_START_ERROR:
            return "hal_start_error";
        default:
            return "unknown";
    }
}
