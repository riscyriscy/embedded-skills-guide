#include <assert.h>
#include <stdio.h>

#include "sensor/ringbuf.h"

/* REQ-001 */
static void keeps_the_newest_samples(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    for (unsigned i = 1; i <= RINGBUF_CAPACITY + 2u; i++) {
        (void)ringbuf_push(&rb, (int16_t)i);
    }
    int16_t oldest = 0;
    assert(ringbuf_get(&rb, 0, &oldest) && oldest == 3);
}

/* REQ-002 */
static void push_reports_the_overwrite(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    for (unsigned i = 0; i < RINGBUF_CAPACITY; i++) {
        assert(!ringbuf_push(&rb, (int16_t)i));
    }
    assert(ringbuf_push(&rb, 99));
}

int main(void)
{
    keeps_the_newest_samples();
    push_reports_the_overwrite();
    puts("test_ring: 2 tests passed");
    return 0;
}
