/* Host test that a precondition fires: the failure handler jumps back into the test. */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "require.h"

static jmp_buf escape;
static int failed_line;

void require_failed(const char *file, int line, const char *expr)
{
    (void)file;
    printf("caught: %s\n", expr);
    failed_line = line;
    longjmp(escape, 1);
}

static unsigned pick(unsigned index)
{
    static const unsigned table[4] = {1, 2, 3, 4};
    REQUIRE(index < 4u);
    return table[index];
}

int main(void)
{
    if (setjmp(escape) == 0) {
        printf("pick(3) = %u\n", pick(3));
        (void)pick(4);
        puts("FAILED: pick(4) returned");
        return 1;
    }
    printf("the precondition fired at line %d\n", failed_line);
    return 0;
}
