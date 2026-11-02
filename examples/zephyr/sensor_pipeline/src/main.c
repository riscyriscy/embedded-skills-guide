#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

#include "busy.h"
#include "trace.h"

/* The pipeline of Chapter 18 in Zephyr: a device interrupt hands samples to a thread through a
   message queue. The sensor thread stands for the device and pends interrupt 31 every 5 ms. The
   consumer needs 12 ms for each sample, so the queue of 3 slots fills. */

#define PERIOD_MS DT_PROP(DT_PATH(zephyr_user), sample_period_ms)
#define WORK_US   DT_PROP(DT_PATH(zephyr_user), consumer_work_us)
#define DEVICE_IRQ 31

struct sample {
	uint32_t seq;
	uint32_t cycles; /* the cycle counter at the interrupt */
};

K_MSGQ_DEFINE(samples, sizeof(struct sample), 3, 4);
static volatile uint32_t produced, dropped;

static void device_isr(const void *arg)
{
	ARG_UNUSED(arg);
	struct sample s = {.seq = ++produced, .cycles = k_cycle_get_32()};
	if (k_msgq_put(&samples, &s, K_NO_WAIT) != 0) {
		dropped++;
	}
}

static void say(const char *text, uint32_t value)
{
	trace(text, true, value);
}

static void sensor_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	k_msleep(2);
	for (int i = 0; i < CONFIG_APP_SAMPLES; i++) {
		NVIC_SetPendingIRQ(DEVICE_IRQ);
		k_msleep(PERIOD_MS);
	}
}

static void consumer_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	uint32_t received = 0;
	struct sample s;
	while (k_msgq_get(&samples, &s, K_MSEC(200)) == 0) {
		received++;
		say("consumer has sample", s.seq);
		if (received == 1u) {
			say("microseconds from the interrupt to the thread",
			    (k_cycle_get_32() - s.cycles) / (sys_clock_hw_cycles_per_sec() / 1000000u));
		}
		busy_us(WORK_US);
	}
	say("samples produced", produced);
	say("samples received", received);
	say("samples dropped by the interrupt", dropped);
	say("queue slots", 3);
	trace_dump();
	sys_reboot(SYS_REBOOT_COLD);
}

K_THREAD_DEFINE(sensor, 1024, sensor_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(consumer, 1024, consumer_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	/* Priority 3 is below the interrupts that the kernel keeps for itself. */
	IRQ_CONNECT(DEVICE_IRQ, 3, device_isr, NULL, 0);
	irq_enable(DEVICE_IRQ);
	return 0;
}
