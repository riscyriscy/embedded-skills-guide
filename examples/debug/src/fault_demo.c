#include <stdint.h>

#include "platform.h"

/* FAULT_KIND selects the mistake: 1 reads an address that has no memory, 2 divides by zero,
   3 jumps to an address with the Arm bit set instead of the Thumb bit. */
#ifndef FAULT_KIND
#define FAULT_KIND 1
#endif

#define SCB_CCR   (*(volatile uint32_t *)0xE000ED14u)
#define SCB_SHCSR (*(volatile uint32_t *)0xE000ED24u)

volatile int fault_kind = FAULT_KIND;
volatile uint32_t divisor;

static uint32_t read_register(volatile const uint32_t *address)
{
    return *address;
}

static uint32_t scale(uint32_t value)
{
    return value / divisor;
}

static void jump_to(uint32_t address)
{
    ((void (*)(void))address)();
}

int main(void)
{
    plat_puts("start");
#ifdef ENABLE_FAULT_HANDLERS
    /* Without these bits a memory, bus or usage fault is escalated to a hard fault. */
    SCB_SHCSR |= (1u << 16) | (1u << 17) | (1u << 18);
#endif
#ifdef TRAP_DIVIDE_BY_ZERO
    /* Without this bit an integer division by zero gives 0 and no fault. */
    SCB_CCR |= (1u << 4);
#endif
    switch (fault_kind) {
    case 1:
        (void)read_register((volatile const uint32_t *)0xA0000000u);
        break;
    case 2:
        (void)scale(100u);
        break;
    case 3:
        jump_to(0x00000100u);
        break;
    default:
        break;
    }
    plat_puts("no fault");
    plat_exit(0);
}
