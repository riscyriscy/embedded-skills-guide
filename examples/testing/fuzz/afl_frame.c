#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "frame.h"

/* A harness for AFL++: it reads one input from standard input, runs the decoder and exits. */
int main(void)
{
    uint8_t input[256];
    uint8_t payload[FRAME_MAX_PAYLOAD];
    size_t payload_len = 0;
    size_t size = fread(input, 1, sizeof input, stdin);
    (void)frame_decode(input, size, payload, sizeof payload, &payload_len);
    return 0;
}
