#include <stddef.h>
int f(int a)
{
    if (a) {
        return 1;
    }
    unreachable();
}
