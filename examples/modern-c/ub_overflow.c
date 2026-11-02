#include <limits.h>

#include "print.h"

/* The programmer wants to know whether x + 1 wraps around. Signed overflow is undefined behaviour,
   so the compiler may assume that it never happens and that x + 1 is always larger than x. */
__attribute__((noinline)) static int wraps(int x)
{
    return x + 1 < x;
}

int main(void)
{
    print_u32("INT_MAX wraps", (uint32_t)wraps(INT_MAX));
    plat_exit(0);
}
