#include "clamp.h"
#include "print.h"

int main(void)
{
    print_u32("clamp", (uint32_t)clamp(50, 10));
    plat_exit(0);
}
