#include "filter/median.h"

static void order(int16_t *lo, int16_t *hi)
{
    if (*lo > *hi) {
        int16_t t = *lo;
        *lo = *hi;
        *hi = t;
    }
}

int16_t filter_median3(const ringbuf_t *rb)
{
    size_t n = ringbuf_count(rb);
    if (n < 3) {
        return 0;
    }
    int16_t a = 0;
    int16_t b = 0;
    int16_t c = 0;
    (void)ringbuf_get(rb, n - 3, &a);
    (void)ringbuf_get(rb, n - 2, &b);
    (void)ringbuf_get(rb, n - 1, &c);
    order(&a, &b);
    order(&b, &c);
    order(&a, &b);
    return b;
}
