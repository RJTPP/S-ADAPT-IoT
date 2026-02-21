#include "app/app_settings.h"
#include <stddef.h>

static uint16_t clamp_u16(uint16_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static uint16_t clamp_step_u16(uint16_t value, uint16_t min_value, uint16_t max_value, uint16_t step)
{
    uint16_t clamped;
    uint16_t offset;
    uint16_t rounded;

    if (step == 0U) {
        return clamp_u16(value, min_value, max_value);
    }

    clamped = clamp_u16(value, min_value, max_value);
    offset = (uint16_t)(clamped - min_value);
    rounded = (uint16_t)(((offset + (step / 2U)) / step) * step);
    if (rounded > (uint16_t)(max_value - min_value)) {
        rounded = (uint16_t)(max_value - min_value);
    }
    return (uint16_t)(min_value + rounded);
}

void app_settings_set_defaults(app_settings_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    cfg->away_mode_enabled = APP_SETTINGS_AWAY_MODE_ENABLED_DEFAULT;
    cfg->flat_mode_enabled = APP_SETTINGS_FLAT_MODE_ENABLED_DEFAULT;
    cfg->away_timeout_s = APP_SETTINGS_AWAY_TIMEOUT_S_DEFAULT;
    cfg->stale_timeout_s = APP_SETTINGS_STALE_TIMEOUT_S_DEFAULT;
    cfg->preoff_dim_s = APP_SETTINGS_PREOFF_DIM_S_DEFAULT;
    cfg->return_band_cm = APP_SETTINGS_RETURN_BAND_CM_DEFAULT;
}

uint8_t app_settings_validate(app_settings_t *cfg)
{
    if (cfg == NULL) {
        return 0U;
    }

    cfg->away_mode_enabled = (cfg->away_mode_enabled != 0U) ? 1U : 0U;
    cfg->flat_mode_enabled = (cfg->flat_mode_enabled != 0U) ? 1U : 0U;
    cfg->away_timeout_s = clamp_step_u16(cfg->away_timeout_s,
                                         APP_SETTINGS_AWAY_TIMEOUT_S_MIN,
                                         APP_SETTINGS_AWAY_TIMEOUT_S_MAX,
                                         APP_SETTINGS_AWAY_TIMEOUT_S_STEP);
    cfg->stale_timeout_s = clamp_step_u16(cfg->stale_timeout_s,
                                          APP_SETTINGS_STALE_TIMEOUT_S_MIN,
                                          APP_SETTINGS_STALE_TIMEOUT_S_MAX,
                                          APP_SETTINGS_STALE_TIMEOUT_S_STEP);
    cfg->preoff_dim_s = clamp_step_u16(cfg->preoff_dim_s,
                                       APP_SETTINGS_PREOFF_DIM_S_MIN,
                                       APP_SETTINGS_PREOFF_DIM_S_MAX,
                                       APP_SETTINGS_PREOFF_DIM_S_STEP);
    cfg->return_band_cm = (uint8_t)clamp_step_u16(cfg->return_band_cm,
                                                  APP_SETTINGS_RETURN_BAND_CM_MIN,
                                                  APP_SETTINGS_RETURN_BAND_CM_MAX,
                                                  APP_SETTINGS_RETURN_BAND_CM_STEP);
    return 1U;
}
