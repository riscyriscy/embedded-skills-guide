#include "unit_record.h"

uint32_t unit_crc32(const void *data, size_t length)
{
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

unit_status_t unit_check(const unit_record_t *record)
{
    const uint8_t *bytes = (const uint8_t *)record;
    size_t erased = 0;
    for (size_t i = 0; i < sizeof *record; i++) {
        erased += bytes[i] == 0xFFu;
    }
    if (erased == sizeof *record) {
        return UNIT_BLANK;
    }
    if (record->magic != UNIT_MAGIC) {
        return UNIT_BAD_MAGIC;
    }
    if (record->version != UNIT_VERSION || record->length != sizeof *record) {
        return UNIT_BAD_VERSION;
    }
    if (unit_crc32(record, sizeof *record - sizeof record->crc) != record->crc) {
        return UNIT_BAD_CRC;
    }
    return UNIT_OK;
}

const char *unit_status_name(unit_status_t status)
{
    switch (status) {
    case UNIT_OK:
        return "ok";
    case UNIT_BLANK:
        return "not provisioned";
    case UNIT_BAD_MAGIC:
        return "bad magic";
    case UNIT_BAD_VERSION:
        return "unknown layout";
    case UNIT_BAD_CRC:
        return "bad check value";
    }
    return "unknown";
}

int32_t unit_correct(const unit_record_t *record, int32_t raw)
{
    int64_t counts = (int64_t)raw + record->adc_offset;
    return (int32_t)(counts * record->adc_gain_ppm / 1000000);
}
