/* The memory protection unit of the Cortex-M4. Region 0 is the flash, read only and
   executable. Region 1 is the RAM, read and write, never executable. Region 2 is a
   256 byte block of RAM that is read only. TEST=1 writes to the read only block,
   TEST=2 jumps to code in RAM. Both end in the MemManage handler. */
#include <stdint.h>

#include "platform.h"

#define REG(address) (*(volatile uint32_t *)(address))
#define SCB_SHCSR  REG(0xE000ED24u)
#define SCB_CFSR   REG(0xE000ED28u)
#define SCB_MMFAR  REG(0xE000ED34u)
#define MPU_TYPE   REG(0xE000ED90u)
#define MPU_CTRL   REG(0xE000ED94u)
#define MPU_RNR    REG(0xE000ED98u)
#define MPU_RBAR   REG(0xE000ED9Cu)
#define MPU_RASR   REG(0xE000EDA0u)

#define RASR_ENABLE   1u
#define RASR_SIZE(n)  ((uint32_t)((n) - 1u) << 1) /* the region has 2^n bytes */
#define RASR_NORMAL   (1u << 18 | 1u << 17) /* shareable, write through */
#define RASR_AP_RW    (3u << 24)            /* read and write, any privilege */
#define RASR_AP_RO    (6u << 24)            /* read only, any privilege */
#define RASR_XN       (1u << 28)            /* no instruction fetch */

#ifndef TEST
#define TEST 1
#endif

static uint8_t block[256] __attribute__((aligned(256)));
static uint16_t ram_code[2] __attribute__((aligned(8), unused)) = {
    0x4770, 0xBF00}; /* bx lr, nop */

static void region(unsigned number, uint32_t base, uint32_t attributes)
{
    MPU_RNR = number;
    MPU_RBAR = base;
    MPU_RASR = attributes | RASR_ENABLE;
}

static void hex(const char *label, uint32_t value)
{
    char line[40];
    unsigned n = 0;
    while (*label != '\0') {
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

void MemManage_Handler(void)
{
    plat_puts("MemManage fault");
    hex("  CFSR  ", SCB_CFSR); /* bit 0: instruction, bit 1: data, bit 7: MMFAR valid */
    if ((SCB_CFSR & 0x80u) != 0u) {
        hex("  MMFAR ", SCB_MMFAR);
    }
    hex("  block is at ", (uint32_t)(uintptr_t)block);
    plat_exit(4);
}

int main(void)
{
    hex("MPU regions: ", (MPU_TYPE >> 8) & 0xFFu);
    region(0, 0x00000000u, RASR_SIZE(19) | RASR_NORMAL | RASR_AP_RO);
    region(1, 0x20000000u, RASR_SIZE(17) | RASR_NORMAL | RASR_AP_RW | RASR_XN);
    region(2, (uint32_t)(uintptr_t)block,
           RASR_SIZE(8) | RASR_NORMAL | RASR_AP_RO | RASR_XN);
    SCB_SHCSR |= 1u << 16; /* enable the MemManage fault, or it is a HardFault */
    MPU_CTRL = 1u | 4u;    /* enable, keep the default map for privileged code */
    __asm__ volatile("dsb\n isb" ::: "memory");
    plat_puts("MPU on");

#if TEST == 1
    plat_puts("write to the read only block");
    block[3] = 1u;
#else
    plat_puts("call code in RAM");
    ((void (*)(void))((uintptr_t)ram_code | 1u))();
#endif
    plat_puts("not reached");
    plat_exit(0);
}
