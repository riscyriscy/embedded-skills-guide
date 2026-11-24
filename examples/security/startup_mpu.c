/* Start-up code for the MPU example: the project start-up with a MemManage, a
   HardFault and a SysTick vector. */
#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
int main(void);

static void Default_Handler(void)
{
    for (;;) {
    }
}

/* The examples define these. A handler that is not defined stays in a loop. */
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

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
    Default_Handler, /* NMI */
    HardFault_Handler,
    MemManage_Handler,
    Default_Handler,
    Default_Handler,
    0, 0, 0, 0,
    Default_Handler, /* SVCall */
    Default_Handler,
    0,
    Default_Handler, /* PendSV */
    SysTick_Handler,
};
