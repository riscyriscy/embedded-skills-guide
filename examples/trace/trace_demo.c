/* A 1 ms tick and a main loop that samples. Every fifth round the main loop writes a
   log line with interrupts off, which delays one tick. The trace shows the delay. */
#include <stdint.h>

#include "platform.h"
#include "trace.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018u)

static volatile uint32_t ticks;

void SysTick_Handler(void)
{
    trace_event(TR_ISR_ENTER);
    ticks++;
    for (volatile unsigned i = 0; i < 40u; i++) {
    }
    trace_event(TR_ISR_EXIT);
}

static void work(unsigned n)
{
    for (volatile unsigned i = 0; i < n; i++) {
    }
}

int main(void)
{
    trace_init();
    SYST_RVR = 25000u - 1u; /* 1 ms at 25 MHz */
    SYST_CVR = 0;
    SYST_CSR = 7u;

    for (unsigned round = 1; ticks < 12u; round++) {
        trace_event(TR_SAMPLE_BEGIN);
        work(900u);
        trace_event(TR_SAMPLE_END);
        if (round % 5u == 0u) {
            trace_event(TR_LOG_BEGIN);
            __asm__ volatile("cpsid i" ::: "memory");
            work(6000u); /* a slow write to a log */
            __asm__ volatile("cpsie i" ::: "memory");
            trace_event(TR_LOG_END);
        }
    }
    trace_dump();
    plat_exit(0);
}
