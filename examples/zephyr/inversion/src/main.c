#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

#include "busy.h"
#include "trace.h"

/* The inversion of Chapter 17 in Zephyr. A low priority thread holds a lock, a high priority thread
   wants it, and a thread of medium priority keeps the processor busy. A smaller number is a higher
   priority in Zephyr. */

#ifdef CONFIG_APP_USE_MUTEX
K_MUTEX_DEFINE(lock);
#define TAKE()    (void)k_mutex_lock(&lock, K_FOREVER)
#define GIVE()    (void)k_mutex_unlock(&lock)
#else
K_SEM_DEFINE(lock, 1, 1);
#define TAKE()    (void)k_sem_take(&lock, K_FOREVER)
#define GIVE()    k_sem_give(&lock)
#endif

static void low_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	TAKE();
	trace("low takes the lock, needs 20 ms of work", false, 0);
	busy_us(20000);
	trace("low gives the lock back", false, 0);
	GIVE();
}

static void medium_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	k_msleep(10);
	trace("medium starts a job of 100 ms", false, 0);
	busy_us(100000);
	trace("medium is done", false, 0);
}

static void high_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	k_msleep(5);
	uint32_t asked = k_cycle_get_32();
	trace("high asks for the lock", false, 0);
	TAKE();
	uint32_t waited = (k_cycle_get_32() - asked) / (sys_clock_hw_cycles_per_sec() / 1000000u);
	trace("high has the lock, microseconds waited", true, waited);
	GIVE();
	k_msleep(150);
	trace_dump();
	sys_reboot(SYS_REBOOT_COLD);
}

K_THREAD_DEFINE(low, 1024, low_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(medium, 1024, medium_thread, NULL, NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(high, 1024, high_thread, NULL, NULL, NULL, 1, 0, 0);

int main(void)
{
	return 0;
}
