#ifndef RT_CYC_H
#define RT_CYC_H

#include <stdint.h>

/* A clock for intervals shorter than one tick, in cycles of the 25 MHz processor clock (40 ns). It
   reads the SysTick counter, which keeps counting while interrupts are masked and while the
   scheduler is suspended, the two states in which the tick count stands still. The interval must be
   shorter than one tick, 25000 cycles. */

#define RT_CYC_PER_TICK 25000u
#define RT_CYC_COUNTER  (*(volatile uint32_t *)0xE000E018u)

static inline uint32_t rt_cyc_now(void)
{
    return RT_CYC_COUNTER;
}

/* Cycles since a reading of rt_cyc_now(). */
static inline uint32_t rt_cyc_since(uint32_t start)
{
    return (start + RT_CYC_PER_TICK - RT_CYC_COUNTER) % RT_CYC_PER_TICK;
}

static inline void rt_cyc_spin(uint32_t cycles)
{
    uint32_t start = rt_cyc_now();
    while (rt_cyc_since(start) < cycles) {
    }
}

#endif
