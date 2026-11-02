#include "platform.h"
#include "sensor/ringbuf.h"
#include "sensor/stats.h"
#include "sensor/version.h"

int main(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);

    for (int16_t i = 1; i <= 20; i++) {
        (void)ringbuf_push(&rb, (int16_t)(i * 10));
    }

    plat_puts("sensor_demo");
    plat_puts(sensor_version());
    /* Capacity 16 keeps samples 50..200, whose mean is 125. */
    plat_puts(stats_mean(&rb) == 125 ? "mean ok" : "mean wrong");
    plat_exit(0);
}
