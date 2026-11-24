/* Registers as a structure of volatile members: Timer0 of the CMSDK, which QEMU models
   for the machine mps2-an386. */
#include <stdint.h>

#include "platform.h"

struct timer_regs {
    volatile uint32_t ctrl;      /* bit 0 enable, bit 3 interrupt enable */
    volatile uint32_t value;     /* current count, counting down */
    volatile uint32_t reload;    /* value loaded when the count reaches zero */
    volatile uint32_t intstatus; /* bit 0 set on zero, cleared by writing 1 */
};
#define TIMER0 ((struct timer_regs *)0x40000000u)
#define TIMER_CTRL_ENABLE 0x1u

int main(void)
{
    TIMER0->reload = 0xFFFFu;
    TIMER0->value = 0xFFFFu;
    TIMER0->ctrl = TIMER_CTRL_ENABLE;

    uint32_t first = TIMER0->value;
    uint32_t second;
    do {
        second = TIMER0->value; /* each read goes to the register */
    } while (second == first);

    plat_puts(second < first ? "the counter counts down" : "the counter counts up");
    plat_exit(0);
}
