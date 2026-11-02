#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "print.h"

typedef struct {
    uint8_t id;
    uint16_t period_ms;
    bool enabled;
} channel_t;

enum { CH_TEMP, CH_PRESSURE, CH_VOLTAGE, CH_COUNT };

/* Designated initializers: the entry is named by its index, the order does not matter,
   and every member that is not named is zero. */
static const channel_t channels[CH_COUNT] = {
    [CH_VOLTAGE] = {.id = 7, .period_ms = 10, .enabled = true},
    [CH_TEMP] = {.id = 1, .period_ms = 1000, .enabled = true},
};

/* A flexible array member: a header followed by data of a length that is known at run time. */
struct packet {
    uint8_t length;
    uint8_t payload[];
};

static uint32_t enabled_period_sum(const channel_t *list, size_t count)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < count; i++) { /* a declaration inside the for statement */
        if (list[i].enabled) {
            sum += list[i].period_ms;
        }
    }
    return sum;
}

/* restrict promises that out and in do not overlap, so the compiler may keep values in registers.
 */
static void scale_by_two(int16_t *restrict out, const int16_t *restrict in, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        out[i] = (int16_t)(in[i] * 2);
    }
}

int main(void)
{
    print_u32("channels", CH_COUNT);
    print_u32("sum of enabled periods", enabled_period_sum(channels, CH_COUNT));
    print_u32("pressure enabled", channels[CH_PRESSURE].enabled);

    /* A compound literal is an unnamed object, here an array of two channels. */
    print_u32("sum for a compound literal",
              enabled_period_sum((const channel_t[]){{.id = 2, .period_ms = 5, .enabled = true},
                                                     {.id = 3, .period_ms = 20, .enabled = true}},
                                 2));

    int16_t in[3] = {100, -200, 300};
    int16_t out[3];
    scale_by_two(out, in, 3);
    print_u32("scaled sample 1 plus 1000", (uint32_t)(out[1] + 1000));
    print_u32("sizeof(struct packet)", sizeof(struct packet));
    plat_exit(0);
}
