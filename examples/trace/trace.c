#include "trace.h"

#include "platform.h"

#define TRACE_LENGTH 128u /* a power of two, so that the index wraps with a mask */

/* Timer1 of the CMSDK counts down at 25 MHz. The trace wants a time that counts up. */
#define TIMER1_CTRL   (*(volatile uint32_t *)0x40001000u)
#define TIMER1_VALUE  (*(volatile uint32_t *)0x40001004u)
#define TIMER1_RELOAD (*(volatile uint32_t *)0x40001008u)

static struct trace_record ring[TRACE_LENGTH];
static volatile uint32_t next; /* total number of records written, never wrapped */

void trace_init(void)
{
    TIMER1_RELOAD = 0xFFFFFFFFu;
    TIMER1_VALUE = 0xFFFFFFFFu;
    TIMER1_CTRL = 1u;
}

void trace_event(enum trace_id id)
{
    uint32_t now = 0xFFFFFFFFu - TIMER1_VALUE;
    /* Reserving the slot is the only step that must be atomic. An interrupt that
       comes after it writes its own slot. */
    uint32_t slot =
        __atomic_fetch_add(&next, 1u, __ATOMIC_RELAXED) & (TRACE_LENGTH - 1u);
    ring[slot].time = now;
    ring[slot].id = (uint32_t)id;
}

static void put(char *out, uint32_t v)
{
    char digits[11];
    unsigned d = 0;
    do {
        digits[d++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    while (d > 0u) {
        *out++ = digits[--d];
    }
    *out = '\0';
}

void trace_dump(void)
{
    uint32_t total = next;
    uint32_t first = total > TRACE_LENGTH ? total - TRACE_LENGTH : 0u;
    for (uint32_t i = first; i < total; i++) {
        const struct trace_record *r = &ring[i & (TRACE_LENGTH - 1u)];
        char line[40] = "T ";
        unsigned n = 2;
        put(&line[n], r->time);
        while (line[n] != '\0') {
            n++;
        }
        line[n++] = ' ';
        put(&line[n], r->id);
        plat_puts(line);
    }
}
