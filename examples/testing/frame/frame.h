#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>

/* A frame on a serial link: sync byte, length, payload, CRC-16/ARC of the length and the payload,
   low byte first. */
#define FRAME_SYNC        0xA5u
#define FRAME_MAX_PAYLOAD 16u
#define FRAME_OVERHEAD    4u /* sync, length and two CRC bytes */

typedef enum {
    FRAME_OK = 0,
    FRAME_ERR_SHORT,  /* fewer bytes than the length field announces */
    FRAME_ERR_SYNC,   /* the first byte is not the sync byte */
    FRAME_ERR_LENGTH, /* the payload does not fit the caller's buffer */
    FRAME_ERR_CRC     /* the check value does not match */
} frame_status_t;

uint16_t frame_crc16(const uint8_t *data, size_t length);

/* Checks a frame and copies its payload to payload[0..payload_cap). */
frame_status_t frame_decode(const uint8_t *in, size_t in_len, uint8_t *payload, size_t payload_cap,
                            size_t *payload_len);

/* Builds a frame. Returns the number of bytes written, or 0 when the payload is longer than
   FRAME_MAX_PAYLOAD or out is too small. */
size_t frame_encode(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_cap);

#endif
