/* The same wait on a status register, written with and without volatile. Compile with -S or -c. */
#include <stdint.h>

struct uart_plain {
    uint32_t status;
    uint32_t data;
};

struct uart_volatile {
    volatile uint32_t status;
    volatile uint32_t data;
};

#define RX_READY 0x1u

uint32_t read_plain(struct uart_plain *u)
{
    while ((u->status & RX_READY) == 0u) {
    }
    return u->data;
}

uint32_t read_volatile(struct uart_volatile *u)
{
    while ((u->status & RX_READY) == 0u) {
    }
    return u->data;
}
