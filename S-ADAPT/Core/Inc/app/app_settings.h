#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

#define APP_SETTINGS_AWAY_MODE_ENABLED_DEFAULT   1U
#define APP_SETTINGS_FLAT_MODE_ENABLED_DEFAULT   1U
#define APP_SETTINGS_AWAY_TIMEOUT_S_DEFAULT      30U
#define APP_SETTINGS_STALE_TIMEOUT_S_DEFAULT     120U
#define APP_SETTINGS_PREOFF_DIM_S_DEFAULT        10U
#define APP_SETTINGS_RETURN_BAND_CM_DEFAULT      10U

#define APP_SETTINGS_AWAY_TIMEOUT_S_MIN          3U
#define APP_SETTINGS_AWAY_TIMEOUT_S_MAX          120U
#define APP_SETTINGS_STALE_TIMEOUT_S_MIN         10U
#define APP_SETTINGS_STALE_TIMEOUT_S_MAX         300U
#define APP_SETTINGS_PREOFF_DIM_S_MIN            2U
#define APP_SETTINGS_PREOFF_DIM_S_MAX            30U
#define APP_SETTINGS_RETURN_BAND_CM_MIN          1U
#define APP_SETTINGS_RETURN_BAND_CM_MAX          30U

typedef struct
{
    uint8_t away_mode_enabled;
    uint8_t flat_mode_enabled;
    uint16_t away_timeout_s;
    uint16_t stale_timeout_s;
    uint16_t preoff_dim_s;
    uint8_t return_band_cm;
} app_settings_t;

void app_settings_set_defaults(app_settings_t *cfg);
uint8_t app_settings_validate(app_settings_t *cfg);

#endif /* APP_SETTINGS_H */
