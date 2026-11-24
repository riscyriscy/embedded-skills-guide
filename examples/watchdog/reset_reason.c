/* The reason for the last reset, kept in a record in RAM that start-up does not clear
   (section .noinit). A part that has a reset status register gives a better answer
   for power-on, pin and brown-out resets. The record adds what no register knows:
   that this program asked for the reset. */
#include <stdint.h>

#include "reset_reason.h"

#define MAGIC 0x52535431u /* "RST1" */
#define STATE_RUNNING  1u
#define STATE_SOFTWARE 2u
#define STATE_FAULT    3u

#define SCB_AIRCR (*(volatile uint32_t *)0xE000ED0Cu)

struct record {
    uint32_t magic;
    uint32_t state;
    uint32_t boots;
    uint32_t check; /* ~(magic ^ state ^ boots): random memory rarely passes */
};
static struct record rec __attribute__((section(".noinit")));

static uint32_t check_of(const struct record *r)
{
    return ~(r->magic ^ r->state ^ r->boots);
}

static void store(uint32_t state)
{
    rec.magic = MAGIC;
    rec.state = state;
    rec.check = check_of(&rec);
}

enum reset_reason rr_init(void)
{
    enum reset_reason reason = RR_POWER_ON;
    if (rec.magic == MAGIC && rec.check == check_of(&rec)) {
        reason = rec.state == STATE_SOFTWARE ? RR_SOFTWARE
                 : rec.state == STATE_FAULT ? RR_FAULT : RR_UNPLANNED;
        rec.boots++;
    } else {
        rec.boots = 1;
    }
    store(STATE_RUNNING);
    return reason;
}

const char *rr_name(enum reset_reason reason)
{
    static const char *const names[] = {
        "power-on", "unplanned (watchdog, lock-up or brown-out)",
        "software request", "fault"};
    return names[reason];
}

unsigned rr_boots(void)
{
    return rec.boots;
}

static void reset_now(void) __attribute__((noreturn));
static void reset_now(void)
{
    SCB_AIRCR = 0x05FA0004u; /* VECTKEY and SYSRESETREQ */
    for (;;) {
    }
}

void rr_request_reset(void)
{
    store(STATE_SOFTWARE);
    reset_now();
}

/* A hard fault records its reason and resets. A product would save the registers
   first (Chapter 9 of Volume 2). */
void HardFault_Handler(void)
{
    store(STATE_FAULT);
    reset_now();
}
