#include <stddef.h>
#include <stdint.h>

#include "frame.h"

/* libFuzzer calls this function with generated input, many thousand times a second. The output
   buffer has the exact size that the decoder is told, so a write past its end is an error that
   AddressSanitizer reports. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t payload[FRAME_MAX_PAYLOAD];
    size_t payload_len = 0;
    (void)frame_decode(data, size, payload, sizeof payload, &payload_len);
    return 0;
}
