#include <stdbool.h>

#include "platform.h"
#include "sensor/ringbuf.h"
#include "sensor/stats.h"

static int failures;

#define CHECK(cond)                                  \
    do {                                             \
        if (cond) {                                  \
            plat_puts("PASS " #cond);                \
        } else {                                     \
            plat_puts("FAIL " #cond);                \
            failures++;                              \
        }                                            \
    } while (0)

int main(void)
{
    ringbuf_t rb;
    int16_t v = 0;

    ringbuf_init(&rb);
    CHECK(ringbuf_count(&rb) == 0);
    CHECK(!ringbuf_get(&rb, 0, &v));
    CHECK(stats_mean(&rb) == 0);

    bool no_overwrite = true;
    for (size_t i = 1; i <= RINGBUF_CAPACITY; i++) {
        no_overwrite = no_overwrite && !ringbuf_push(&rb, (int16_t)i);
    }
    CHECK(no_overwrite);
    CHECK(ringbuf_count(&rb) == RINGBUF_CAPACITY);
    CHECK(ringbuf_push(&rb, 100));          /* overwrites sample 1 */
    CHECK(ringbuf_get(&rb, 0, &v) && v == 2);
    CHECK(ringbuf_get(&rb, RINGBUF_CAPACITY - 1, &v) && v == 100);
    CHECK(!ringbuf_get(&rb, RINGBUF_CAPACITY, &v));

    plat_exit(failures == 0 ? 0 : 1);
}
