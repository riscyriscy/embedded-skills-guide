#include <stdint.h>

#include "platform.h"

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
int main(void);

/* The registers that the processor pushes when it takes an exception, and the fault status
 * registers. */
struct fault_dump {
    uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
    uint32_t cfsr, hfsr, mmfar, bfar;
    uint32_t valid;
};
struct fault_dump fault_dump;

#define SCB_CFSR  (*(volatile uint32_t *)0xE000ED28u)
#define SCB_HFSR  (*(volatile uint32_t *)0xE000ED2Cu)
#define SCB_MMFAR (*(volatile uint32_t *)0xE000ED34u)
#define SCB_BFAR  (*(volatile uint32_t *)0xE000ED38u)

static void put_hex(const char *label, uint32_t value)
{
    char line[40];
    unsigned n = 0;
    while (*label != '\0' && n < 20u) {
        line[n++] = *label++;
    }
    line[n++] = '0';
    line[n++] = 'x';
    for (int shift = 28; shift >= 0; shift -= 4) {
        line[n++] = "0123456789abcdef"[(value >> shift) & 0xFu];
    }
    line[n] = '\0';
    plat_puts(line);
}

/* Called from the vector with the address of the stacked registers. It copies them and the status
   registers to a variable that a debugger or a crash handler can read, prints one line for each,
   and stops the program. */
void fault_report(const uint32_t *frame)
{
    fault_dump.r0 = frame[0];
    fault_dump.r1 = frame[1];
    fault_dump.r2 = frame[2];
    fault_dump.r3 = frame[3];
    fault_dump.r12 = frame[4];
    fault_dump.lr = frame[5];
    fault_dump.pc = frame[6];
    fault_dump.psr = frame[7];
    fault_dump.cfsr = SCB_CFSR;
    fault_dump.hfsr = SCB_HFSR;
    fault_dump.mmfar = SCB_MMFAR;
    fault_dump.bfar = SCB_BFAR;
    fault_dump.valid = 1;
    plat_puts("FAULT");
    put_hex("pc    ", fault_dump.pc);
    put_hex("lr    ", fault_dump.lr);
    put_hex("cfsr  ", fault_dump.cfsr);
    put_hex("hfsr  ", fault_dump.hfsr);
    put_hex("bfar  ", fault_dump.bfar);
    plat_exit(1);
}

/* Chooses the stack that was in use when the fault happened (bit 2 of LR) and passes it on. */
__attribute__((naked)) static void Fault_Handler(void)
{
    __asm__ volatile("tst lr, #4\n"
                     "ite eq\n"
                     "mrseq r0, msp\n"
                     "mrsne r0, psp\n"
                     "b fault_report\n");
}

static void Default_Handler(void)
{
    for (;;) {
    }
}

void Reset_Handler(void)
{
    const uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata;) {
        *dst++ = *src++;
    }
    for (uint32_t *dst = &_sbss; dst < &_ebss;) {
        *dst++ = 0;
    }
    (void)main();
    Default_Handler();
}

typedef void (*vector_t)(void);

__attribute__((section(".isr_vector"), used)) const vector_t vector_table[16] = {
    (vector_t)&_estack,
    Reset_Handler,
    Default_Handler,
    Fault_Handler, /* HardFault */
    Fault_Handler, /* MemManage */
    Fault_Handler, /* BusFault */
    Fault_Handler, /* UsageFault */
    0,
    0,
    0,
    0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    Default_Handler,
};
