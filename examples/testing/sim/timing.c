#include <stdint.h>

#include "print.h"

#define DEMCR      (*(volatile uint32_t *)0xE000EDFCu)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018u)

static volatile uint32_t sink;

static void work(void)
{
    for (uint32_t i = 0; i < 10000u; i++) {
        sink += i;
    }
}

int main(void)
{
    DEMCR |= 1u << 24;
    DWT_CTRL |= 1u;
    uint32_t c0 = DWT_CYCCNT;
    work();
    uint32_t c1 = DWT_CYCCNT;
    print_u32("DWT cycle counter after the loop", c1);
    print_u32("DWT cycles for the loop", c1 - c0);

    SYST_RVR = 0xFFFFFFu;
    SYST_CSR = 5u; /* enable, processor clock */
    uint32_t s0 = SYST_CVR;
    work();
    uint32_t s1 = SYST_CVR;
    print_u32("SysTick counts for the loop", (s0 - s1) & 0xFFFFFFu);
    plat_exit(0);
}
