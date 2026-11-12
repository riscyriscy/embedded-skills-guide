#include "wdt.h"

#define WDOG(offset) (*(volatile uint32_t *)(0x40008000u + (offset)))
#define WDOGLOAD    0x000u
#define WDOGCONTROL 0x008u
#define WDOGINTCLR  0x00Cu /* a write reloads the counter and clears the interrupt */
#define WDOGLOCK    0xC00u
#define CONTROL_INTEN 1u
#define CONTROL_RESEN 2u
#define UNLOCK_KEY    0x1ACCE551u

static volatile uint32_t seen;

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
    seen |= 1u << bit;
}

void wdt_service(uint32_t required)
{
    if ((seen & required) == required) {
        seen = 0;
        wdt_feed();
    }
}
