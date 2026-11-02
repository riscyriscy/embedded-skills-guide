#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

/* Interrupt to thread latency, the job of freertos/demos/latency.c. A timer pends interrupt 31 every
   10 ms. The handler reads the SysTick counter and gives a semaphore, and the thread reads the
   counter again. */

#define DEVICE_IRQ 31
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018u)
#define RELOAD     25000u

K_SEM_DEFINE(wake, 0, 1);
static volatile uint32_t stamp;

static void device_isr(const void *arg)
{
	ARG_UNUSED(arg);
	stamp = SYST_CVR;
	k_sem_give(&wake);
}

static void pend(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	NVIC_SetPendingIRQ(DEVICE_IRQ);
}

K_TIMER_DEFINE(device, pend, NULL);

int main(void)
{
	IRQ_CONNECT(DEVICE_IRQ, 3, device_isr, NULL, 0);
	irq_enable(DEVICE_IRQ);
	k_timer_start(&device, K_MSEC(30), K_MSEC(10));

	uint32_t min = 0xFFFFFFFFu, max = 0, total = 0;
	for (int i = 0; i < 8; i++) {
		(void)k_sem_take(&wake, K_FOREVER);
		uint32_t cycles = (stamp + RELOAD - SYST_CVR) % RELOAD;
		if (cycles < min) {
			min = cycles;
		}
		if (cycles > max) {
			max = cycles;
		}
		total += cycles;
	}
	printk("interrupt to thread, least, in cycles of 25 MHz: %u\n", min);
	printk("interrupt to thread, greatest, in cycles of 25 MHz: %u\n", max);
	printk("interrupt to thread, mean, in cycles of 25 MHz: %u\n", total / 8u);
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}
