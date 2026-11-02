#include "sensor/stats.h"

int16_t stats_mean(const ringbuf_t *rb)
{
    size_t n = ringbuf_count(rb);
    if (n == 0) {
        return 0;
    }
    int32_t sum = 0;
    for (size_t i = 0; i < n; i++) {
        int16_t v = 0;
        (void)ringbuf_get(rb, i, &v);
        sum += v;
    }
    return (int16_t)(sum / (int32_t)n);
}
