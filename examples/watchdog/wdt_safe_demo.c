/* A reset loop guard: a task that is always stuck makes the watchdog reset the device
   again and again. After MAX_RESETS unplanned resets in a row the program stops
   starting the application and enters a safe mode. */
#include <stdint.h>

#include "platform.h"
#include "reset_reason.h"
#include "wdt.h"

#define LOAD 0x00100000u /* 41.9 ms a period */
#define MAX_RESETS 3u

static void run_application(void)
{
    wdt_start(LOAD);
    for (;;) {
        wdt_progress(0u);
        /* task 1 never reports, as if it waited for a sensor that is not there */
        wdt_service(3u);
    }
}

int main(void)
{
    enum reset_reason why = rr_init();
    unsigned boots = rr_boots();

    if (why == RR_UNPLANNED && boots > MAX_RESETS) {
        plat_puts("safe mode: the application stopped answering the watchdog "
                  "3 times in a row");
        plat_exit(0);
    }
    plat_puts(boots == 1u ? "start the application"
                          : "restart the application after an unplanned reset");
    run_application();
}
