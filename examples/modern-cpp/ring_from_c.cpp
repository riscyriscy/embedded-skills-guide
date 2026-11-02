#include "print.h"
#include "sensor/ringbuf.h" /* a C header, with no extern "C" */

int main()
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    ringbuf_push(&rb, 5);
    print_u32("count", ringbuf_count(&rb));
    plat_exit(0);
}
