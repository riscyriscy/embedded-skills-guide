#ifndef PRINT_H
#define PRINT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "platform.h"
#ifdef __cplusplus
}
#endif

/* Writes "label: value" with plat_puts, so that the examples need no printf and run the same
   on the host and on the Cortex-M4. */
static inline void print_u32(const char *label, uint32_t value)
{
    char line[64];
    unsigned n = 0;
    while (*label != '\0' && n < 40u) {
        line[n++] = *label++;
    }
    line[n++] = ':';
    line[n++] = ' ';
    char digits[11];
    unsigned d = 0;
    do {
        digits[d++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (d > 0u) {
        line[n++] = digits[--d];
    }
    line[n] = '\0';
    plat_puts(line);
}

#endif
