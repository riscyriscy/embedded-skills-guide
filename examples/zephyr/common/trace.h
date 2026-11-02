#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* Output through the UART takes real time: the model of the UART in QEMU sends one character in the
   time of one character at the baud rate. A program that prints while it is measured changes what
   it measures. These helpers store each line in memory and print all of them at the end. */
#define TRACE_LINES 32

struct trace_line {
	uint32_t ms;
	const char *text;
	uint32_t value;
	bool has_value;
};

static struct trace_line trace_buffer[TRACE_LINES];
static volatile unsigned trace_count;

static inline void trace(const char *text, bool has_value, uint32_t value)
{
	unsigned key = irq_lock();
	if (trace_count < TRACE_LINES) {
		trace_buffer[trace_count++] = (struct trace_line){k_uptime_get_32(), text, value, has_value};
	}
	irq_unlock(key);
}

static inline void trace_dump(void)
{
	for (unsigned i = 0; i < trace_count; i++) {
		if (trace_buffer[i].has_value) {
			printk("t=%3u ms  %s: %u\n", trace_buffer[i].ms, trace_buffer[i].text, trace_buffer[i].value);
		} else {
			printk("t=%3u ms  %s\n", trace_buffer[i].ms, trace_buffer[i].text);
		}
	}
}

#endif
