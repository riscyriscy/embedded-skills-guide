#include <stdint.h>

#include "print.h"

int main(void)
{
    uint8_t mask = 0x0F;
    uint8_t high = 0x80;

    /* Operands of type uint8_t are promoted to int before the operator is applied. */
    print_u32("~mask == 0xF0", ~mask == 0xF0);
    print_u32("(uint8_t)~mask == 0xF0", (uint8_t)~mask == 0xF0);
    print_u32("(high << 1) == 0", (high << 1) == 0);
    print_u32("(uint8_t)(high << 1) == 0", (uint8_t)(high << 1) == 0);
    plat_exit(0);
}
