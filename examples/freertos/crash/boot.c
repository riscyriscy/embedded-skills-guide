/* The bootloader: reports why it was started, then starts the application at APP_BASE. */
#include <stdint.h>

#include "crash.h"
#include "platform.h"

#define APP_BASE 0x00008000u
#define SCB_VTOR (*(volatile uint32_t *)0xE000ED08u)

static void say(const char *a, const char *b)
{
    char line[80];
    unsigned n = 0;
    for (; *a != '\0' && n < 40u; a++) {
        line[n++] = *a;
    }
    for (; *b != '\0' && n < 79u; b++) {
        line[n++] = *b;
    }
    line[n] = '\0';
    plat_puts(line);
}

static void start_app(void)
{
    const uint32_t *vectors = (const uint32_t *)APP_BASE;
    SCB_VTOR = APP_BASE;
    __asm volatile("msr msp, %0\n"
                   "bx %1\n" ::"r"(vectors[0]),
                   "r"(vectors[1]));
    for (;;) {
    }
}

int main(void)
{
    /* crash_peek does not change the record: the application reads and clears it. */
    say("bootloader: last reset was ", crash_class_name(crash_peek()));
    start_app();
}
