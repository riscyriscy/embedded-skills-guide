#include <stdckdint.h>
int f(int a, int b, int *r)
{
    return ckd_add(r, a, b);
}
