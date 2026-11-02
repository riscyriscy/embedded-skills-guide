#ifndef BUSY_H
#define BUSY_H

#include <stdint.h>
#include <zephyr/kernel.h>

/* Spends about this many microseconds of processor time. Time in which another thread or an
   interrupt handler ran is not counted: a step of more than 40 us between two reads of the
   cycle counter means that something else ran. */
static inline void busy_us(uint32_t microseconds)
{
	uint32_t per_us = sys_clock_hw_cycles_per_sec() / 1000000u;
	uint32_t target = microseconds * per_us;
	uint32_t last = k_cycle_get_32();
	uint32_t spent = 0;
	while (spent < target) {
		uint32_t now = k_cycle_get_32();
		uint32_t step = now - last;
		last = now;
		if (step < 40u * per_us) {
			spent += step;
		}
	}
}

#endif
