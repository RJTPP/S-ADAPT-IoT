#include "ultrasonic.h"

#include "main.h"

static TIM_HandleTypeDef *s_echo_tim = NULL;
static uint32_t s_echo_channel = TIM_CHANNEL_2;

static uint32_t capture_flag_from_channel(uint32_t channel)
{
    switch (channel) {
        case TIM_CHANNEL_1:
            return TIM_FLAG_CC1;
        case TIM_CHANNEL_2:
            return TIM_FLAG_CC2;
        case TIM_CHANNEL_3:
            return TIM_FLAG_CC3;
        case TIM_CHANNEL_4:
            return TIM_FLAG_CC4;
        default:
            return 0U;
    }
}

static uint8_t wait_for_capture(uint32_t capture_flag, uint32_t timeout_us)
{
    uint32_t t0;

    if (s_echo_tim == NULL || capture_flag == 0U) {
        return 0U;
    }

    t0 = __HAL_TIM_GET_COUNTER(s_echo_tim);
    while (__HAL_TIM_GET_FLAG(s_echo_tim, capture_flag) == RESET) {
        if ((uint32_t)(__HAL_TIM_GET_COUNTER(s_echo_tim) - t0) > timeout_us) {
            return 0U;
        }
    }
    return 1U;
}

static void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(s_echo_tim, 0U);
    while (__HAL_TIM_GET_COUNTER(s_echo_tim) < us) {
    }
}

void ultrasonic_init(TIM_HandleTypeDef *tim, uint32_t channel)
{
    s_echo_tim = tim;
    s_echo_channel = channel;

    if (s_echo_tim == NULL) {
        return;
    }

    HAL_TIM_Base_Start(s_echo_tim);
    HAL_TIM_IC_Start(s_echo_tim, s_echo_channel);
}

uint32_t ultrasonic_read_echo_us(uint32_t timeout_us)
{
    uint32_t start;
    uint32_t stop;
    uint32_t capture_flag;
    uint32_t period;

    if (s_echo_tim == NULL) {
        return 0U;
    }

    capture_flag = capture_flag_from_channel(s_echo_channel);
    if (capture_flag == 0U) {
        return 0U;
    }

    __HAL_TIM_SET_COUNTER(s_echo_tim, 0U);
    __HAL_TIM_SET_CAPTUREPOLARITY(s_echo_tim, s_echo_channel, TIM_INPUTCHANNELPOLARITY_RISING);
    __HAL_TIM_CLEAR_FLAG(s_echo_tim, capture_flag);

    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
    delay_us(2U);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
    delay_us(10U);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

    if (!wait_for_capture(capture_flag, timeout_us)) {
        return 0U;
    }
    start = HAL_TIM_ReadCapturedValue(s_echo_tim, s_echo_channel);

    __HAL_TIM_CLEAR_FLAG(s_echo_tim, capture_flag);
    __HAL_TIM_SET_CAPTUREPOLARITY(s_echo_tim, s_echo_channel, TIM_INPUTCHANNELPOLARITY_FALLING);

    if (!wait_for_capture(capture_flag, timeout_us)) {
        __HAL_TIM_SET_CAPTUREPOLARITY(s_echo_tim, s_echo_channel, TIM_INPUTCHANNELPOLARITY_RISING);
        return 0U;
    }
    stop = HAL_TIM_ReadCapturedValue(s_echo_tim, s_echo_channel);
    __HAL_TIM_SET_CAPTUREPOLARITY(s_echo_tim, s_echo_channel, TIM_INPUTCHANNELPOLARITY_RISING);

    if (stop >= start) {
        return stop - start;
    }

    period = __HAL_TIM_GET_AUTORELOAD(s_echo_tim);
    return ((period - start) + stop + 1U);
}

uint32_t ultrasonic_read_distance_cm(uint32_t timeout_us, uint32_t error_value_cm)
{
    uint32_t echo_us = ultrasonic_read_echo_us(timeout_us);

    if (echo_us == 0U) {
        return error_value_cm;
    }

    return echo_us / 58U;
}
