#include "controller.h"

#include <stddef.h>

/* The whole behaviour in one table: row = state, column = event, value = next state. */
static const uint8_t next_state[CTL_STATE_COUNT][CTL_EVENT_COUNT] = {
    [CTL_IDLE]     = {[EV_START] = CTL_WARMUP},
    [CTL_WARMUP]   = {[EV_READY] = CTL_SAMPLING, [EV_STOP] = CTL_IDLE},
    [CTL_SAMPLING] = {[EV_FAULT] = CTL_FAULT, [EV_STOP] = CTL_IDLE},
    [CTL_FAULT]    = {[EV_RESET] = CTL_IDLE},
};

void ctl_init(struct controller *c, const struct adc_ops *adc)
{
    c->state = CTL_IDLE;
    c->bad_in_row = 0;
    c->last = 0;
    c->adc = adc;
}

enum ctl_state ctl_handle(struct controller *c, enum ctl_event ev)
{
    if (ev >= CTL_EVENT_COUNT) {
        return c->state;
    }
    enum ctl_state next = (enum ctl_state)next_state[c->state][ev];
    if (next != CTL_NONE) {
        c->state = next;
        c->bad_in_row = 0;
    }
    return c->state;
}

void ctl_step(struct controller *c)
{
    if (c->state != CTL_SAMPLING) {
        return;
    }
    int16_t value = 0;
    if (c->adc->read(c->adc->ctx, &value) != 0 || value < 0 || value > 4095) {
        c->bad_in_row++;
        if (c->bad_in_row >= CTL_MAX_BAD) {
            (void)ctl_handle(c, EV_FAULT);
        }
        return;
    }
    c->bad_in_row = 0;
    c->last = value;
}

const char *ctl_state_name(enum ctl_state s)
{
    static const char *const names[CTL_STATE_COUNT] = {"none", "idle", "warmup",
                                                       "sampling", "fault"};
    return s < CTL_STATE_COUNT ? names[s] : "?";
}
