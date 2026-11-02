#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
int main(void);

/* Defined by the linker script: the table of the constructors of the C++ objects. */
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

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
    for (void (**ctor)(void) = __init_array_start; ctor < __init_array_end; ctor++) {
        (*ctor)();
    }
    (void)main();
    Default_Handler();
}

typedef void (*vector_t)(void);

/* The first word is the initial stack pointer, the second the reset vector. */
__attribute__((section(".isr_vector"), used)) const vector_t vector_table[16] = {
    (vector_t)&_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
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
