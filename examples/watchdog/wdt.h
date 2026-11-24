#ifndef WDT_H
#define WDT_H

#include <stdint.h>

/* The watchdog of the Arm CMSDK, which the machine mps2-an386 models. It counts down
   from a load value at 25 MHz and resets the system when the count reaches zero. */
#define WDT_CLOCK_HZ 25000000u

void wdt_start(uint32_t load);
void wdt_feed(void);

/* Progress flags: the watchdog is fed only when every required part of the program
   has reported since the last feed. wdt_service is called from the main loop, and
   each task calls wdt_progress(bit). */
void wdt_progress(unsigned bit);
void wdt_service(uint32_t required);

#endif
