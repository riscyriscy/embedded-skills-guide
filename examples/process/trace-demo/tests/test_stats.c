#include <assert.h>
#include <stdio.h>

#include "sensor/ringbuf.h"
#include "sensor/stats.h"

/* REQ-004 */
static void mean_of_nothing_is_zero(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    assert(stats_mean(&rb) == 0);
}

/* REQ-005 */
static void mean_rounds_toward_zero(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    (void)ringbuf_push(&rb, -3);
    (void)ringbuf_push(&rb, -2);
    assert(stats_mean(&rb) == -2); /* -2.5 becomes -2 */
}

/* REQ-040 */
static void mean_of_one_sample(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    (void)ringbuf_push(&rb, 7);
    assert(stats_mean(&rb) == 7);
}

int main(void)
{
    mean_of_nothing_is_zero();
    mean_rounds_toward_zero();
    mean_of_one_sample();
    puts("test_stats: 3 tests passed");
    return 0;
}
