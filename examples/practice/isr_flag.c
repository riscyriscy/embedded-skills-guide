/* A SysTick interrupt counts, and main waits for the fifth tick. Build without options
   for the broken program and with -DTICK_VOLATILE for the correct one. */
#include <stdint.h>

#include "platform.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018u)

#ifdef TICK_VOLATILE
static volatile uint32_t ticks;
#else
static uint32_t ticks;
#endif

void SysTick_Handler(void)
{
    ticks++;
}

int main(void)
{
    SYST_RVR = 0x0000FFFFu;
    SYST_CVR = 0;
    SYST_CSR = 7u; /* enable, interrupt, processor clock */
    while (ticks < 5u) {
    }
    plat_puts("5 ticks seen");
    plat_exit(0);
}
