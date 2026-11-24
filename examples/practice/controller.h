#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

/* Zero is not a state: the transition table uses 0 for "no transition", so an event
   that a state does not list is ignored and the state stays. */
enum ctl_state { CTL_NONE, CTL_IDLE, CTL_WARMUP, CTL_SAMPLING,
                 CTL_FAULT, CTL_STATE_COUNT };
enum ctl_event { EV_START = 1, EV_READY, EV_FAULT, EV_STOP, EV_RESET, CTL_EVENT_COUNT };

/* The hardware seen from the controller: one function and its context. A target
   supplies the real converter and a test supplies a fake. */
struct adc_ops {
    int (*read)(void *ctx, int16_t *value); /* 0 on success, negative on failure */
    void *ctx;
};

#define CTL_MAX_BAD 3u /* bad samples in a row before the state CTL_FAULT */

struct controller {
    enum ctl_state state;
    unsigned bad_in_row;
    int16_t last;
    const struct adc_ops *adc;
};

void ctl_init(struct controller *c, const struct adc_ops *adc);
enum ctl_state ctl_handle(struct controller *c, enum ctl_event ev);
/* Reads one sample when sampling, and turns a run of bad samples into EV_FAULT. */
void ctl_step(struct controller *c);
const char *ctl_state_name(enum ctl_state s);

#endif
