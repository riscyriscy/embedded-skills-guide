#include "frame.h"

#include <string.h>

uint16_t frame_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1u) != 0u ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

frame_status_t frame_decode(const uint8_t *in, size_t in_len, uint8_t *payload, size_t payload_cap,
                            size_t *payload_len)
{
    if (in_len < FRAME_OVERHEAD) {
        return FRAME_ERR_SHORT;
    }
    if (in[0] != FRAME_SYNC) {
        return FRAME_ERR_SYNC;
    }
    size_t length = in[1];
    if (length > payload_cap) {
        return FRAME_ERR_LENGTH;
    }
    if (in_len < FRAME_OVERHEAD + length) {
        return FRAME_ERR_SHORT;
    }
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    /* A fuzzer cannot guess a check value, so fuzzing builds skip this test. */
    uint16_t sent = (uint16_t)(in[2 + length] | (in[3 + length] << 8));
    if (sent != frame_crc16(&in[1], 1u + length)) {
        return FRAME_ERR_CRC;
    }
#endif
    memcpy(payload, &in[2], length);
    *payload_len = length;
    return FRAME_OK;
}

size_t frame_encode(const uint8_t *payload, size_t payload_len, uint8_t *out, size_t out_cap)
{
    if (payload_len > FRAME_MAX_PAYLOAD || out_cap < FRAME_OVERHEAD + payload_len) {
        return 0;
    }
    out[0] = FRAME_SYNC;
    out[1] = (uint8_t)payload_len;
    memcpy(&out[2], payload, payload_len);
    uint16_t crc = frame_crc16(&out[1], 1u + payload_len);
    out[2 + payload_len] = (uint8_t)(crc & 0xFFu);
    out[3 + payload_len] = (uint8_t)(crc >> 8);
    return FRAME_OVERHEAD + payload_len;
}
