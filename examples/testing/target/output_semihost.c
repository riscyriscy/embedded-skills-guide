#include "unity_config.h"

#define SYS_WRITEC 0x03

/* Writes one character to the console of the debugger or of QEMU. */
void unity_putc(int c)
{
    char ch = (char)c;
    register int r0 __asm__("r0") = SYS_WRITEC;
    register const void *r1 __asm__("r1") = &ch;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
}
