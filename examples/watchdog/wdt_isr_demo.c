/* The mistake: the watchdog is fed from a timer interrupt, so it is fed while the
   main loop does nothing. */
#include <stdint.h>

#include "platform.h"
#include "wdt.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u)

static volatile uint32_t ticks;

void SysTick_Handler(void)
{
    ticks++;
    wdt_feed(); /* answers the watchdog whatever the main loop is doing */
}

int main(void)
{
    wdt_start(0x00100000u);      /* 41.9 ms */
    SYST_RVR = 250000u - 1u;     /* 10 ms at 25 MHz */
    SYST_CSR = 7u;               /* clock, interrupt and enable */
    while (ticks < 50u) {
        /* The main loop makes no progress at all. */
    }
    plat_puts("50 ticks of 10 ms with a stuck main loop, and the watchdog never reset "
              "the device");
    plat_exit(0);
}
