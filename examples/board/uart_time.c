/* How long does the simulator take to send 80 characters through the UART, in SysTick counts. */
#include <stdint.h>

#include "platform.h"

#define UART0_BASE        0x40004000u
#define UART_DATA         (*(volatile uint32_t *)(UART0_BASE + 0x00u))
#define UART_STATE        (*(volatile uint32_t *)(UART0_BASE + 0x04u))
#define UART_CTRL         (*(volatile uint32_t *)(UART0_BASE + 0x08u))
#define UART_BAUDDIV      (*(volatile uint32_t *)(UART0_BASE + 0x10u))
#define SYST_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYST_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYST_CVR          (*(volatile uint32_t *)0xE000E018u)

static void uart_putc(char c)
{
    while ((UART_STATE & 1u) != 0u) { /* wait while the transmit register is full */
    }
    UART_DATA = (uint32_t)c;
}

int main(void)
{
    char digits[11];
    UART_BAUDDIV = 16u;
    UART_CTRL = 1u;
    SYST_RVR = 0xFFFFFFu;
    SYST_CSR = 5u; /* enable, processor clock */
    uint32_t t0 = SYST_CVR;
    for (int i = 0; i < 79; i++) {
        uart_putc('x');
    }
    uart_putc('\n');
    uint32_t counts = (t0 - SYST_CVR) & 0xFFFFFFu;
    int n = 0;
    do {
        digits[n++] = (char)('0' + counts % 10u);
        counts /= 10u;
    } while (counts != 0u);
    plat_puts("80 characters sent, SysTick counts used:");
    char text[11];
    for (int i = 0; i < n; i++) {
        text[i] = digits[n - 1 - i];
    }
    text[n] = '\0';
    plat_puts(text);
    plat_exit(0);
}
