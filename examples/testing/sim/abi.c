#include <stddef.h>
#include <stdint.h>

#include "print.h"

/* Prints the sizes and conventions that differ between a host and the Cortex-M4. Run it in both
   places with examples/run.sh and compare. */
enum color { RED, GREEN, BLUE };

int main(void)
{
    print_u32("sizeof(int)", sizeof(int));
    print_u32("sizeof(long)", sizeof(long));
    print_u32("sizeof(long long)", sizeof(long long));
    print_u32("sizeof(void *)", sizeof(void *));
    print_u32("sizeof(size_t)", sizeof(size_t));
    print_u32("sizeof(enum color)", sizeof(enum color));
    print_u32("sizeof(long double)", sizeof(long double));
    char c = (char)200;
    print_u32("char is signed (1 yes, 0 no)", c < 0 ? 1u : 0u);
    uint32_t probe = 1;
    print_u32("little endian (1 yes, 0 no)", *(volatile uint8_t *)&probe == 1u ? 1u : 0u);
    plat_exit(0);
}
