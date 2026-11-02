#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "print.h"

/* A register that is seen as a whole word and as named fields. The anonymous struct is C11.
   The order of the bit fields is chosen by the compiler, so the assertion below checks it. */
typedef union {
    uint32_t word;
    struct {
        uint32_t enable : 1;
        uint32_t mode : 3;
        uint32_t : 4;
        uint32_t divider : 8;
        uint32_t : 16;
    };
} ctrl_reg_t;

_Static_assert(sizeof(ctrl_reg_t) == 4, "ctrl_reg_t must be one 32 bit register");

struct header {
    uint8_t type;
    uint32_t value;
};
_Static_assert(offsetof(struct header, value) == 4, "value must be at offset 4");

/* _Generic selects an expression by the type of its argument, at compile time. */
#define BITS_OF(x) _Generic((x), uint8_t: 8, uint16_t: 16, uint32_t: 32, default: 0)

_Alignas(8) static uint8_t buffer[16];

static atomic_uint_fast32_t ticks;

int main(void)
{
    ctrl_reg_t reg = {.word = 0};
    reg.enable = 1;
    reg.mode = 5;
    reg.divider = 0x20;
    print_u32("register word", reg.word);

    uint8_t a = 0;
    uint16_t b = 0;
    uint32_t c = 0;
    print_u32("bits of uint8_t", (uint32_t)BITS_OF(a));
    print_u32("bits of uint16_t", (uint32_t)BITS_OF(b));
    print_u32("bits of uint32_t", (uint32_t)BITS_OF(c));
    print_u32("buffer address modulo 8", (uint32_t)((uintptr_t)buffer % 8u));

    (void)atomic_fetch_add(&ticks, 1u);
    (void)atomic_fetch_add(&ticks, 1u);
    print_u32("ticks", (uint32_t)atomic_load(&ticks));
    plat_exit(0);
}
