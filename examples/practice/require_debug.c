/* Policy for development: say where, and stop with a status that a script can see. */
#include "platform.h"
#include "require.h"

static void put_number(char *out, unsigned n)
{
    char digits[11];
    unsigned d = 0;
    do {
        digits[d++] = (char)('0' + n % 10u);
        n /= 10u;
    } while (n != 0u);
    while (d > 0u) {
        *out++ = digits[--d];
    }
    *out = '\0';
}

void require_failed(const char *file, int line, const char *expr)
{
    char text[128];
    unsigned n = 0;
    for (const char *s = "require failed: "; *s != '\0' && n < 100u; s++) {
        text[n++] = *s;
    }
    for (; *file != '\0' && n < 100u; file++) {
        text[n++] = *file;
    }
    text[n++] = ':';
    put_number(&text[n], (unsigned)line);
    plat_puts(text);
    plat_puts(expr);
    plat_exit(70);
}
