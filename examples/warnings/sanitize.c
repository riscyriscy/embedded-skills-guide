#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "print.h"

int main(void)
{
    int16_t samples[8] = {0};
    volatile unsigned index = 8; /* one past the end */
    samples[index] = 1;          /* a write outside the array */

    uint8_t *heap = malloc(4);
    heap[4] = 2; /* a write outside an allocation */
    free(heap);

    print_u32("done", 1);
    plat_exit(0);
}
