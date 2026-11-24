/* Policy for a product: keep the place of the failure in the record that survives the reset, then reset.
   The record is read by the next boot (Chapter 13 of Volume 2). */
#include <stdint.h>

#include "require.h"

#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0Cu)

struct assert_record {
    uint32_t magic;
    const char *file; /* a pointer into flash: valid after the reset because the image is the same */
    uint32_t line;
};
struct assert_record last_assert __attribute__((section(".noinit")));

void require_failed(const char *file, int line, const char *expr)
{
    (void)expr; /* the text of the condition costs flash in every REQUIRE, so the product keeps the line only */
    last_assert.magic = 0x41535254u; /* "ASRT" */
    last_assert.file = file;
    last_assert.line = (uint32_t)line;
    SCB_AIRCR = 0x05FA0004u;
    for (;;) {
    }
}
