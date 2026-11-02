#include "print.h"

extern "C" {
#include "sensor/ringbuf.h"
}

int main()
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    ringbuf_push(&rb, 5);
    print_u32("count", ringbuf_count(&rb));
    plat_exit(0);
}
