#ifndef RESET_REASON_H
#define RESET_REASON_H

enum reset_reason {
    RR_POWER_ON,  /* the memory held no record: a cold start */
    RR_UNPLANNED, /* the program was running: watchdog, lock-up or brown-out */
    RR_SOFTWARE,  /* rr_request_reset was called */
    RR_FAULT,     /* the fault handler asked for the reset */
};

/* Reads the record that the last run left, classifies the reset and starts a new
   record. Call it once, first thing in main. */
enum reset_reason rr_init(void);
const char *rr_name(enum reset_reason reason);
unsigned rr_boots(void); /* resets since the last power-on, counting this one */

/* Marks the next reset as deliberate and resets the processor. */
void rr_request_reset(void) __attribute__((noreturn));

#endif
