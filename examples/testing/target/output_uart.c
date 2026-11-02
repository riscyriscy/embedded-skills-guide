#include <stdint.h>

#include "unity_config.h"

/* UART0 of the CMSDK UART in the QEMU machine mps2-an386. A board has its own registers here. */
#define UART0_BASE        0x40004000u
#define UART_DATA         (*(volatile uint32_t *)(UART0_BASE + 0x00u))
#define UART_STATE        (*(volatile uint32_t *)(UART0_BASE + 0x04u))
#define UART_CTRL         (*(volatile uint32_t *)(UART0_BASE + 0x08u))
#define UART_BAUDDIV      (*(volatile uint32_t *)(UART0_BASE + 0x10u))
#define UART_STATE_TXFULL 1u
#define UART_CTRL_TXEN    1u

void unity_putc(int c)
{
    static int ready;
    if (!ready) {
        UART_BAUDDIV = 16u;
        UART_CTRL = UART_CTRL_TXEN;
        ready = 1;
    }
    while ((UART_STATE & UART_STATE_TXFULL) != 0u) {
    }
    UART_DATA = (uint32_t)c;
}
