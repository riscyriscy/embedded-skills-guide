#include "print.h"

[[nodiscard]] static int read_sensor(int *out)
{
    *out = 21;
    return 0; /* zero means success */
}

int main(void)
{
    int value = 0;
    read_sensor(&value); /* the status is ignored */
    print_u32("value", (uint32_t)value);
    plat_exit(0);
}
