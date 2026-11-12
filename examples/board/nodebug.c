/* What a program that prints through semihosting does on a board with no debugger attached.
   The hard fault handler writes to the UART, so that the result can be seen. */
#include <stdint.h>

#include "platform.h"

#define UART0_BASE        0x40004000u
#define UART_DATA         (*(volatile uint32_t *)(UART0_BASE + 0x00u))
#define UART_STATE        (*(volatile uint32_t *)(UART0_BASE + 0x04u))
#define UART_CTRL         (*(volatile uint32_t *)(UART0_BASE + 0x08u))
#define UART_BAUDDIV      (*(volatile uint32_t *)(UART0_BASE + 0x10u))

static void uart_puts(const char *s)
{
    UART_BAUDDIV = 16u;
    UART_CTRL = 1u;
    for (; *s != '\0'; s++) {
        while ((UART_STATE & 1u) != 0u) {
        }
        UART_DATA = (uint32_t)*s;
    }
}

void HardFault_Handler(void)
{
    uart_puts("hard fault: the breakpoint of the semihosting call found no debugger\n");
    for (;;) {
    }
}

int main(void)
{
    plat_puts("hello through semihosting");
    plat_exit(0);
}
