#include "platform.h"

#include <stdint.h>

#define SYS_WRITE0                   0x04
#define SYS_EXIT_EXTENDED            0x20
#define ADP_STOPPED_APPLICATION_EXIT 0x20026u

static int semihost(int op, const void *arg)
{
    register int r0 __asm__("r0") = op;
    register const void *r1 __asm__("r1") = arg;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

void plat_puts(const char *text)
{
    (void)semihost(SYS_WRITE0, text);
    (void)semihost(SYS_WRITE0, "\n");
}

void plat_exit(int status)
{
    const uint32_t block[2] = {ADP_STOPPED_APPLICATION_EXIT, (uint32_t)status};
    (void)semihost(SYS_EXIT_EXTENDED, block);
    for (;;) {
    }
}
