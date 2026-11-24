#ifndef UNIT_RECORD_H
#define UNIT_RECORD_H

#include <stddef.h>
#include <stdint.h>

/* The data that is different on every unit. The factory writes it into the last page of the flash,
   which the image never uses and an update never writes. Little endian, 52 bytes. */
#define UNIT_MAGIC   0x54494E55u /* the bytes "UNIT" */
#define UNIT_VERSION 1u

typedef struct {
    uint32_t magic;
    uint16_t version;      /* layout of this record */
    uint16_t length;       /* bytes of the record, the check value included */
    char serial[16];       /* text, padded with zeros */
    uint8_t mac[6];
    uint8_t hw_rev;        /* revision of the board */
    uint8_t reserved;
    int32_t adc_offset;    /* calibration: counts added to the raw reading */
    uint32_t adc_gain_ppm; /* calibration: 1000000 is a gain of 1.0 */
    uint8_t key_id[8];     /* first 8 bytes of the SHA-256 of the public key of the unit */
    uint32_t crc;          /* CRC-32 of every byte before this one */
} unit_record_t;

_Static_assert(sizeof(unit_record_t) == 52, "the layout is part of the interface with the factory");

typedef enum {
    UNIT_OK,
    UNIT_BLANK,       /* erased flash: the factory has not written this unit */
    UNIT_BAD_MAGIC,
    UNIT_BAD_VERSION,
    UNIT_BAD_CRC,
} unit_status_t;

uint32_t unit_crc32(const void *data, size_t length);
unit_status_t unit_check(const unit_record_t *record);
const char *unit_status_name(unit_status_t status);

/* The reading of the converter after the calibration of this unit. */
int32_t unit_correct(const unit_record_t *record, int32_t raw);

#endif
