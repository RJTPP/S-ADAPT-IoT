#include "support/settings_store.h"

#include "stm32l4xx_hal.h"

#include <stddef.h>
#include <string.h>

#define SETTINGS_FLASH_BASE_ADDR   0x0803F800UL
#define SETTINGS_FLASH_PAGE_SIZE   0x800UL
#define SETTINGS_FLASH_END_ADDR    (SETTINGS_FLASH_BASE_ADDR + SETTINGS_FLASH_PAGE_SIZE)
#define SETTINGS_FLASH_BANK        FLASH_BANK_1
#define SETTINGS_FLASH_PAGE_INDEX  ((SETTINGS_FLASH_BASE_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE)

#define SETTINGS_RECORD_MAGIC      0x53414450UL
#define SETTINGS_RECORD_VERSION    1U

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t payload_len;
    uint32_t seq;
    app_settings_t payload;
    uint32_t crc32;
    uint32_t reserved;
} settings_record_t;

typedef struct
{
    uint8_t has_valid;
    uint8_t has_empty_slot;
    uint32_t max_seq;
    uint32_t next_write_addr;
    settings_record_t latest_valid;
} settings_scan_result_t;

_Static_assert((sizeof(settings_record_t) % 8U) == 0U, "settings_record_t must align to doubleword");

static uint8_t settings_is_in_range(const app_settings_t *cfg)
{
    if (cfg == NULL) {
        return 0U;
    }

    if ((cfg->away_mode_enabled > 1U) || (cfg->flat_mode_enabled > 1U)) {
        return 0U;
    }
    if ((cfg->away_timeout_s < APP_SETTINGS_AWAY_TIMEOUT_S_MIN) ||
        (cfg->away_timeout_s > APP_SETTINGS_AWAY_TIMEOUT_S_MAX) ||
        (((cfg->away_timeout_s - APP_SETTINGS_AWAY_TIMEOUT_S_MIN) % APP_SETTINGS_AWAY_TIMEOUT_S_STEP) != 0U)) {
        return 0U;
    }
    if ((cfg->stale_timeout_s < APP_SETTINGS_STALE_TIMEOUT_S_MIN) ||
        (cfg->stale_timeout_s > APP_SETTINGS_STALE_TIMEOUT_S_MAX) ||
        (((cfg->stale_timeout_s - APP_SETTINGS_STALE_TIMEOUT_S_MIN) % APP_SETTINGS_STALE_TIMEOUT_S_STEP) != 0U)) {
        return 0U;
    }
    if ((cfg->preoff_dim_s < APP_SETTINGS_PREOFF_DIM_S_MIN) ||
        (cfg->preoff_dim_s > APP_SETTINGS_PREOFF_DIM_S_MAX) ||
        (((cfg->preoff_dim_s - APP_SETTINGS_PREOFF_DIM_S_MIN) % APP_SETTINGS_PREOFF_DIM_S_STEP) != 0U)) {
        return 0U;
    }
    if ((cfg->return_band_cm < APP_SETTINGS_RETURN_BAND_CM_MIN) ||
        (cfg->return_band_cm > APP_SETTINGS_RETURN_BAND_CM_MAX) ||
        (((cfg->return_band_cm - APP_SETTINGS_RETURN_BAND_CM_MIN) % APP_SETTINGS_RETURN_BAND_CM_STEP) != 0U)) {
        return 0U;
    }

    return 1U;
}

static uint32_t settings_crc32_compute(const uint8_t *data, size_t size)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i;

    for (i = 0U; i < size; i++) {
        uint32_t byte_value = data[i];
        uint8_t bit;

        crc ^= byte_value;
        for (bit = 0U; bit < 8U; bit++) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1UL);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }

    return ~crc;
}

static uint8_t settings_slot_is_erased(uint32_t address)
{
    const uint64_t *slot_data = (const uint64_t *)(uintptr_t)address;
    size_t words = sizeof(settings_record_t) / sizeof(uint64_t);
    size_t i;

    for (i = 0U; i < words; i++) {
        if (slot_data[i] != UINT64_C(0xFFFFFFFFFFFFFFFF)) {
            return 0U;
        }
    }

    return 1U;
}

static uint8_t settings_record_is_valid(const settings_record_t *record)
{
    uint32_t expected_crc;

    if (record == NULL) {
        return 0U;
    }
    if (record->magic != SETTINGS_RECORD_MAGIC) {
        return 0U;
    }
    if (record->version != SETTINGS_RECORD_VERSION) {
        return 0U;
    }
    if (record->payload_len != sizeof(app_settings_t)) {
        return 0U;
    }

    expected_crc = settings_crc32_compute((const uint8_t *)record, offsetof(settings_record_t, crc32));
    if (expected_crc != record->crc32) {
        return 0U;
    }

    return 1U;
}

static void settings_scan_records(settings_scan_result_t *out_scan)
{
    uint32_t address;

    memset(out_scan, 0, sizeof(*out_scan));
    out_scan->next_write_addr = SETTINGS_FLASH_BASE_ADDR;

    for (address = SETTINGS_FLASH_BASE_ADDR;
         (address + sizeof(settings_record_t)) <= SETTINGS_FLASH_END_ADDR;
         address += sizeof(settings_record_t)) {
        settings_record_t record;

        if (settings_slot_is_erased(address) != 0U) {
            if (out_scan->has_empty_slot == 0U) {
                out_scan->has_empty_slot = 1U;
                out_scan->next_write_addr = address;
            }
            continue;
        }

        memcpy(&record, (const void *)(uintptr_t)address, sizeof(record));
        if (settings_record_is_valid(&record) == 0U) {
            continue;
        }

        if ((out_scan->has_valid == 0U) || (record.seq > out_scan->max_seq)) {
            out_scan->has_valid = 1U;
            out_scan->max_seq = record.seq;
            out_scan->latest_valid = record;
        }
    }

    if (out_scan->has_empty_slot == 0U) {
        out_scan->next_write_addr = SETTINGS_FLASH_END_ADDR;
    }
}

static settings_store_status_t settings_flash_erase_page(void)
{
    FLASH_EraseInitTypeDef erase_cfg;
    uint32_t page_error = 0U;

    memset(&erase_cfg, 0, sizeof(erase_cfg));
    erase_cfg.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_cfg.Banks = SETTINGS_FLASH_BANK;
    erase_cfg.Page = SETTINGS_FLASH_PAGE_INDEX;
    erase_cfg.NbPages = 1U;

    if (HAL_FLASHEx_Erase(&erase_cfg, &page_error) != HAL_OK) {
        return SETTINGS_STORE_FLASH_ERASE_FAIL;
    }

    return SETTINGS_STORE_OK;
}

static settings_store_status_t settings_flash_write_record(uint32_t address, const settings_record_t *record)
{
    size_t offset;

    for (offset = 0U; offset < sizeof(settings_record_t); offset += sizeof(uint64_t)) {
        uint64_t data64;

        memcpy(&data64, ((const uint8_t *)record) + offset, sizeof(uint64_t));
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address + offset, data64) != HAL_OK) {
            return SETTINGS_STORE_FLASH_PROGRAM_FAIL;
        }
    }

    if (memcmp((const void *)(uintptr_t)address, record, sizeof(settings_record_t)) != 0) {
        return SETTINGS_STORE_VERIFY_FAIL;
    }

    return SETTINGS_STORE_OK;
}

settings_store_status_t settings_store_load(app_settings_t *out_cfg, uint8_t *used_defaults)
{
    settings_scan_result_t scan;

    if (out_cfg == NULL) {
        return SETTINGS_STORE_NULL_PTR;
    }

    if (used_defaults != NULL) {
        *used_defaults = 0U;
    }

    settings_scan_records(&scan);
    if (scan.has_valid == 0U) {
        app_settings_set_defaults(out_cfg);
        if (used_defaults != NULL) {
            *used_defaults = 1U;
        }
        return SETTINGS_STORE_NO_VALID_RECORD;
    }

    *out_cfg = scan.latest_valid.payload;
    (void)app_settings_validate(out_cfg);
    return SETTINGS_STORE_OK;
}

settings_store_status_t settings_store_save(const app_settings_t *cfg)
{
    settings_scan_result_t scan;
    settings_record_t record;
    settings_store_status_t status;
    uint32_t target_address;
    uint32_t next_seq = 1U;

    if (cfg == NULL) {
        return SETTINGS_STORE_NULL_PTR;
    }
    if (settings_is_in_range(cfg) == 0U) {
        return SETTINGS_STORE_INVALID_CFG;
    }

    settings_scan_records(&scan);
    if (scan.has_valid != 0U) {
        next_seq = scan.max_seq + 1U;
    }

    target_address = scan.next_write_addr;

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return SETTINGS_STORE_FLASH_UNLOCK_FAIL;
    }

    if ((target_address + sizeof(settings_record_t)) > SETTINGS_FLASH_END_ADDR) {
        status = settings_flash_erase_page();
        if (status != SETTINGS_STORE_OK) {
            (void)HAL_FLASH_Lock();
            return status;
        }
        target_address = SETTINGS_FLASH_BASE_ADDR;
    }

    memset(&record, 0, sizeof(record));
    record.magic = SETTINGS_RECORD_MAGIC;
    record.version = SETTINGS_RECORD_VERSION;
    record.payload_len = sizeof(app_settings_t);
    record.seq = next_seq;
    record.payload = *cfg;
    record.reserved = 0xFFFFFFFFUL;
    record.crc32 = settings_crc32_compute((const uint8_t *)&record, offsetof(settings_record_t, crc32));

    status = settings_flash_write_record(target_address, &record);
    (void)HAL_FLASH_Lock();
    return status;
}

settings_store_status_t settings_store_reset_defaults(app_settings_t *out_cfg)
{
    if (out_cfg == NULL) {
        return SETTINGS_STORE_NULL_PTR;
    }

    app_settings_set_defaults(out_cfg);
    (void)app_settings_validate(out_cfg);
    return SETTINGS_STORE_OK;
}
