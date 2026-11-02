#include <stdint.h>

#include "platform.h"

/* The same two jobs as demos/preempt.c, on the bare processor with a super loop. A job of 200 us
   must start every 10 ms. A job of 30 ms runs every 100 ms. The loop checks the clock and runs
   the job whose time has come, one at a time, to the end. */

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018u)

static volatile uint32_t ms;

void SysTick_Handler(void)
{
    ms++;
}

static uint32_t now_us(void)
{
    uint32_t m, value;
    do {
        m = ms;
        value = SYST_CVR;
    } while (m != ms);
    return m * 1000u + (24999u - value) / 25u;
}

static void busy_us(uint32_t microseconds)
{
    uint32_t start = now_us();
    while (now_us() - start < microseconds) {
    }
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
    SYST_RVR = 24999u; /* 25 MHz and 1 ms */
    SYST_CSR = 7u;     /* enable, interrupt, processor clock */

    uint32_t next_fast = 10000u, next_slow = 5000u, runs = 0, worst = 0;
    while (next_fast <= 1000000u) {
        uint32_t now = now_us();
        if (now >= next_fast) {
            uint32_t late = now - next_fast;
            if (late > worst) {
                worst = late;
            }
            busy_us(200u);
            runs++;
            next_fast += 10000u;
        } else if (now >= next_slow) {
            busy_us(30000u);
            next_slow += 100000u;
        }
    }
    say("fast job runs in one second", runs);
    say("worst delay of the start of the fast job, microseconds", worst);
    plat_exit(0);
}
