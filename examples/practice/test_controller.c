/* Host test of the controller with a fake converter that returns a scripted list of results. */
#include <stdio.h>

#include "controller.h"

struct script {
    const int *results; /* each entry: 0 to 4095 is a sample, -1 is a read error */
    unsigned count, pos;
};

static int fake_read(void *ctx, int16_t *value)
{
    struct script *s = ctx;
    int r = s->results[s->pos < s->count ? s->pos : s->count - 1u];
    s->pos++;
    if (r < 0) {
        return -1;
    }
    *value = (int16_t)r;
    return 0;
}

static int failures, checks;
#define CHECK(cond)                                                                   \
    do {                                                                              \
        checks++;                                                                     \
        if (!(cond)) {                                                                \
            failures++;                                                               \
            printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);                  \
        }                                                                             \
    } while (0)

static void event(struct controller *c, enum ctl_event ev, const char *name)
{
    enum ctl_state before = c->state;
    enum ctl_state after = ctl_handle(c, ev);
    printf("%-8s -%s-> %s\n", ctl_state_name(before), name, ctl_state_name(after));
}

int main(void)
{
    static const int results[] = {100, 2000, 5000, -1, 7, -1, -1, -1};
    struct script s = {results, 8, 0};
    struct adc_ops adc = {fake_read, &s};
    struct controller c;
    ctl_init(&c, &adc);

    event(&c, EV_READY, "ready"); /* not allowed in idle: ignored */
    CHECK(c.state == CTL_IDLE);
    event(&c, EV_START, "start");
    CHECK(c.state == CTL_WARMUP);
    event(&c, EV_READY, "ready");
    CHECK(c.state == CTL_SAMPLING);

    for (unsigned i = 0; i < 8; i++) {
        ctl_step(&c);
        printf("step %u: %-8s bad_in_row=%u last=%d\n", i, ctl_state_name(c.state), c.bad_in_row, c.last);
    }
    CHECK(c.state == CTL_FAULT);
    CHECK(c.last == 7); /* the last good sample is kept */
    event(&c, EV_STOP, "stop"); /* not allowed in fault: ignored */
    CHECK(c.state == CTL_FAULT);
    event(&c, EV_RESET, "reset");
    CHECK(c.state == CTL_IDLE);

    printf("%d checks, %d failed\n", checks, failures);
    return failures != 0;
}
