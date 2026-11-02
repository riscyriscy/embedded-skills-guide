#ifndef RT_H
#define RT_H

#include <stdint.h>

/* Helpers that the demonstration programs share. All output goes through the semihosting call of
   the platform layer, one whole line at a time. */

/* Writes "t=<ms> <text>" with the time of the call in milliseconds. */
void rt_say(const char *text);
/* Writes "t=<ms> <label>: <text>". */
void rt_say_str(const char *label, const char *text);
/* Writes "t=<ms> <label>: <value>". */
void rt_say_u32(const char *label, uint32_t value);
/* Time in microseconds since the scheduler started: the tick count and the SysTick counter. */
uint32_t rt_now_us(void);
/* Spends about this many microseconds of processor time, not counting time that the task spends
   preempted. */
void rt_busy_us(uint32_t microseconds);
/* Ends the program with an exit status after the lines are written. */
void rt_finish(int status) __attribute__((noreturn));

/* Interrupt 0 and 1 of the device: enable, give them a priority and pend them from software. */
void rt_irq_setup(uint32_t irq, uint8_t priority);
void rt_irq_pend(uint32_t irq);

#endif
