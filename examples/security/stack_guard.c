/* A function that copies a name of a length that the caller gives into a buffer of 8
   bytes. Build with and without -fstack-protector-strong. */
#include <stddef.h>
#include <stdint.h>

#include "platform.h"

/* The compiler puts this value below the saved registers of a function with a buffer,
   and compares it before the function returns. A product sets it from a random
   number generator at start-up. */
uintptr_t __stack_chk_guard = 0x5A17C0DEu;

void __stack_chk_fail(void)
{
    plat_puts("stack smashing detected, the function did not return");
    plat_exit(3);
}

__attribute__((noinline)) static unsigned copy_name(const char *src, size_t n)
{
    volatile char buf[8]; /* volatile: the compiler must keep every store */
    for (size_t i = 0; i < n; i++) {
        buf[i] = src[i];
    }
    plat_puts("name copied"); /* a call, so the return address is saved in the frame */
    return (unsigned char)buf[0];
}

int main(void)
{
    static const char name[32] = "0123456789abcdef0123456789abcde";
    plat_puts("copy 8 bytes");
    (void)copy_name(name, 8);
    plat_puts("copy 8 bytes: returned");
    plat_puts("copy 24 bytes");
    (void)copy_name(name, 24);
    plat_puts("copy 24 bytes: returned");
    plat_exit(0);
}
