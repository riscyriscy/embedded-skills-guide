#ifndef WDT_H
#define WDT_H

#include <stdint.h>

/* The watchdog of the Arm CMSDK, which the machine mps2-an386 models. It counts down
   from a load value at 25 MHz. At the first zero it raises an interrupt (the NMI on this
   machine). At the second zero it resets the system, unless a write to WDOGINTCLR has
   reloaded the counter and cleared the interrupt in between. A load of N counts therefore
   resets the system after 2 * N / 25 MHz. */
#define WDT_CLOCK_HZ 25000000u

void wdt_start(uint32_t load);
void wdt_feed(void);

/* Progress flags: the watchdog is fed only when every required part of the program
   has reported since the last feed. wdt_service is called from the main loop, and
   each task or interrupt handler calls wdt_progress(bit). Both functions use atomic
   operations on the flags, so a report that arrives while wdt_service runs is not lost. */
void wdt_progress(unsigned bit);
void wdt_service(uint32_t required);

#endif
