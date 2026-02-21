#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <stdint.h>

#include "app/app_settings.h"

typedef enum
{
    SETTINGS_STORE_OK = 0,
    SETTINGS_STORE_NULL_PTR,
    SETTINGS_STORE_INVALID_CFG,
    SETTINGS_STORE_NO_VALID_RECORD,
    SETTINGS_STORE_FLASH_UNLOCK_FAIL,
    SETTINGS_STORE_FLASH_ERASE_FAIL,
    SETTINGS_STORE_FLASH_PROGRAM_FAIL,
    SETTINGS_STORE_VERIFY_FAIL,
    SETTINGS_STORE_CRC_FAIL
} settings_store_status_t;

settings_store_status_t settings_store_load(app_settings_t *out_cfg, uint8_t *used_defaults);
settings_store_status_t settings_store_save(const app_settings_t *cfg);
settings_store_status_t settings_store_reset_defaults(app_settings_t *out_cfg);

#endif /* SETTINGS_STORE_H */
