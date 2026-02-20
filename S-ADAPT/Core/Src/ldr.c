#include "ldr.h"

static ADC_HandleTypeDef *s_ldr_adc = NULL;

void ldr_init(ADC_HandleTypeDef *hadc)
{
    s_ldr_adc = hadc;
}

ldr_status_t ldr_read_raw(uint16_t *out_raw)
{
    if (s_ldr_adc == NULL) {
        return LDR_STATUS_NOT_INIT;
    }
    if (out_raw == NULL) {
        return LDR_STATUS_NULL_PTR;
    }

    if (HAL_ADC_Start(s_ldr_adc) != HAL_OK) {
        return LDR_STATUS_START_ERROR;
    }

    if (HAL_ADC_PollForConversion(s_ldr_adc, 5U) != HAL_OK) {
        (void)HAL_ADC_Stop(s_ldr_adc);
        return LDR_STATUS_TIMEOUT;
    }

    *out_raw = (uint16_t)HAL_ADC_GetValue(s_ldr_adc);

    if (HAL_ADC_Stop(s_ldr_adc) != HAL_OK) {
        return LDR_STATUS_STOP_ERROR;
    }

    return LDR_STATUS_OK;
}

const char *ldr_status_to_string(ldr_status_t status)
{
    switch (status) {
        case LDR_STATUS_OK:
            return "ok";
        case LDR_STATUS_NOT_INIT:
            return "not_init";
        case LDR_STATUS_NULL_PTR:
            return "null_ptr";
        case LDR_STATUS_START_ERROR:
            return "start_error";
        case LDR_STATUS_TIMEOUT:
            return "timeout";
        case LDR_STATUS_STOP_ERROR:
            return "stop_error";
        default:
            return "unknown";
    }
}
