/* A function with a precondition, called with a value that breaks it. */
#include <stdint.h>

#include "platform.h"
#include "require.h"

static uint16_t table[8] = {10, 20, 30, 40, 50, 60, 70, 80};

static uint16_t lookup(unsigned index)
{
    REQUIRE(index < 8u);
    return table[index];
}

int main(void)
{
    plat_puts(lookup(7) == 80u ? "lookup(7) is 80" : "lookup(7) is wrong");
    (void)lookup(8);
    plat_puts("not reached");
    plat_exit(0);
}
