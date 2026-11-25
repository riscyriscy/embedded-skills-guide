#include "wdt.h"

#define WDOG(offset) (*(volatile uint32_t *)(0x40008000u + (offset)))
#define WDOGLOAD    0x000u
#define WDOGCONTROL 0x008u
#define WDOGINTCLR  0x00Cu /* a write reloads the counter, clears the interrupt */
#define WDOGLOCK    0xC00u
#define CONTROL_INTEN 1u
#define CONTROL_RESEN 2u
#define UNLOCK_KEY    0x1ACCE551u

static uint32_t seen;

void wdt_start(uint32_t load)
{
    WDOG(WDOGLOCK) = UNLOCK_KEY;
    WDOG(WDOGLOAD) = load;
    WDOG(WDOGCONTROL) = CONTROL_INTEN | CONTROL_RESEN;
}

void wdt_feed(void)
{
    WDOG(WDOGINTCLR) = 1u;
}

void wdt_progress(unsigned bit)
{
    __atomic_fetch_or(&seen, 1u << bit, __ATOMIC_RELAXED);
}

/* Takes the required flags and clears them in one atomic step, so that a report from an
   interrupt between the test and the clear stays set for the next period. When some flag
   was missing, the flags taken are put back. */
void wdt_service(uint32_t required)
{
    uint32_t taken = __atomic_fetch_and(&seen, ~required, __ATOMIC_RELAXED) & required;
    if (taken == required) {
        wdt_feed();
    } else {
        __atomic_fetch_or(&seen, taken, __ATOMIC_RELAXED);
    }
}
