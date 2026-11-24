/* Formats one number into a buffer in three ways. Select the way with -DMODE=0, 1 or 2. */
#include <stdio.h>

volatile int reading = 1234;
volatile float volts = 3.3f;
char text[32];

int main(void)
{
#if MODE == 0 /* hand-written conversion of an unsigned decimal */
    unsigned v = (unsigned)reading;
    char *p = text + sizeof text - 1;
    *p = '\0';
    do {
        *--p = (char)('0' + v % 10u);
        v /= 10u;
    } while (v != 0u);
    return p[0];
#elif MODE == 1 /* snprintf with an integer */
    snprintf(text, sizeof text, "%d", reading);
#else /* snprintf with a float: needs -u _printf_float with newlib-nano */
    snprintf(text, sizeof text, "%.2f", (double)volts);
#endif
    return text[0];
}
