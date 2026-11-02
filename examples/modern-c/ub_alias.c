#include <stdint.h>
#include <string.h>

#include "print.h"

/* Writes a float and an int through pointers of different types, then reads the int back. */
__attribute__((noinline)) static int32_t store_both(int32_t *i, float *f)
{
    *i = 1;
    *f = 2.0f;
    return *i;
}

/* The defined way to see the bits of a float. */
static uint32_t bits_of(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    return u;
}

int main(void)
{
    int32_t shared = 0;
    print_u32("through two pointer types", (uint32_t)store_both(&shared, (float *)&shared));
    print_u32("bits of 2.0f by memcpy", bits_of(2.0f));
    plat_exit(0);
}
