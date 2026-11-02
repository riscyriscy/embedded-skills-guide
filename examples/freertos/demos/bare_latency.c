#include <stdint.h>

#include "platform.h"

/* The job of latency.c on the bare processor. The interrupt handler stores the SysTick counter and
   a flag, and the main loop waits for the flag. */

#define SYST_CSR   (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018u)
#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define NVIC_ISPR0 (*(volatile uint32_t *)0xE000E200u)
#define RELOAD     25000u

static volatile uint32_t stamp, flag, ticks;

void SysTick_Handler(void)
{
    if (++ticks % 10u == 0u && ticks > 20u) {
        NVIC_ISPR0 = 1u;
    }
}

void Irq0_Handler(void)
{
    stamp = SYST_CVR;
    flag = 1;
}

static void say(const char *label, uint32_t value)
{
    char line[80];
    unsigned n = 0;
    while (*label != '\0' && n < 60u) {
        line[n++] = *label++;
    }
    line[n++] = ':';
    line[n++] = ' ';
    char digits[11];
    unsigned d = 0;
    do {
        digits[d++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (d > 0u) {
        line[n++] = digits[--d];
    }
    line[n] = '\0';
    plat_puts(line);
}

int main(void)
{
    SYST_RVR = RELOAD - 1u;
    SYST_CSR = 7u;
    NVIC_ISER0 = 1u;
    uint32_t min = 0xFFFFFFFFu, max = 0, total = 0;
    for (int i = 0; i < 8; i++) {
        while (flag == 0u) {
        }
        flag = 0;
        uint32_t cycles = (stamp + RELOAD - SYST_CVR) % RELOAD;
        if (cycles < min) {
            min = cycles;
        }
        if (cycles > max) {
            max = cycles;
        }
        total += cycles;
    }
    say("interrupt to main loop, least, in cycles of 25 MHz", min);
    say("interrupt to main loop, greatest, in cycles of 25 MHz", max);
    say("interrupt to main loop, mean, in cycles of 25 MHz", total / 8u);
    plat_exit(0);
}
