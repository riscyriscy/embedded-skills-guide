/* Four resets in one run: a stuck task, a hard fault, a software request and the end of the demo. */
#include <stdint.h>

#include "platform.h"
#include "reset_reason.h"
#include "wdt.h"

#define LOAD 0x00100000u /* 1048576 counts of 25 MHz: 41.9 ms */
#define TASK_A 0u
#define TASK_B 1u

static void say(const char *what, unsigned n, const char *text)
{
    char line[96];
    unsigned i = 0;
    for (; *what != '\0'; i++, what++) {
        line[i] = *what;
    }
    line[i++] = ' ';
    line[i++] = (char)('0' + n);
    line[i++] = ':';
    line[i++] = ' ';
    for (; *text != '\0' && i < sizeof line - 1u; i++, text++) {
        line[i] = *text;
    }
    line[i] = '\0';
    plat_puts(line);
}

int main(void)
{
    enum reset_reason why = rr_init();
    unsigned boot = rr_boots();
    say("boot", boot, rr_name(why));

    switch (why) {
    case RR_POWER_ON:
        /* Two tasks report progress. Task B stops after 5 rounds, as if it were stuck in a wait. */
        wdt_start(LOAD);
        for (unsigned round = 0;; round++) {
            wdt_progress(TASK_A);
            if (round < 5u) {
                wdt_progress(TASK_B);
            }
            wdt_service((1u << TASK_A) | (1u << TASK_B));
            for (volatile uint32_t i = 0; i < 2000u; i++) {
            }
            if (round == 5u) {
                plat_puts("task B stopped, task A runs on");
            }
        }
    case RR_UNPLANNED:
        plat_puts("read from an address with no memory");
        return (int)*(volatile uint32_t *)0xA0000000u;
    case RR_FAULT:
        plat_puts("asking for a reset");
        rr_request_reset();
    case RR_SOFTWARE:
        plat_exit(0);
    }
    return 1;
}
