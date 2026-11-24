/* A loop that pushes a sample and asks for the mean, 20000 times, to give a profiler something to find. */
#include <stdint.h>
#include <stdio.h>

#include "sensor/ringbuf.h"
#include "sensor/stats.h"

#ifdef FAST
/* The order of the samples does not change a sum, so the loop reads the array and skips the call of
   ringbuf_get and its bounds check for each sample. */
static int16_t mean_fast(const ringbuf_t *rb)
{
    if (rb->count == 0) {
        return 0;
    }
    int32_t sum = 0;
    for (size_t i = 0; i < rb->count; i++) {
        sum += rb->data[i];
    }
    return (int16_t)(sum / (int32_t)rb->count);
}
#define stats_mean mean_fast
#endif

int main(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);
    int32_t total = 0;
    for (int i = 0; i < 20000; i++) {
        (void)ringbuf_push(&rb, (int16_t)(i % 1000));
        total += stats_mean(&rb);
    }
    printf("sum of means: %d\n", (int)total);
    return 0;
}
