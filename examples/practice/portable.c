/* Two operations whose code depends on the core: an atomic increment and an integer division. */
#include <stdatomic.h>

atomic_uint counter;

unsigned bump(void)
{
    return atomic_fetch_add(&counter, 1u);
}

int divide(int a, int b)
{
    return a / b;
}
