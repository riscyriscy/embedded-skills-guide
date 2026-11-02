#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One defect per case. The case is chosen on the command line, so that every tool runs the same
   binary against the same bug. The index and the offsets come from run time values, so the
   compiler cannot see the defect and remove or reject it. */

static int table[4] = {1, 2, 3, 4};

static int heap_overflow(int n)
{
    char *p = malloc(8);
    memset(p, 0, 8);
    p[7 + n] = 'x'; /* one byte past the allocation */
    int r = p[0];
    free(p);
    return r;
}

static int stack_overflow(int n)
{
    char buf[8];
    memset(buf, 0, sizeof buf);
    buf[7 + n] = 'x'; /* one byte past the array */
    return buf[0];
}

static int global_overflow(int n)
{
    return table[3 + n]; /* one element past the array */
}

static int use_after_free(int n)
{
    int *p = malloc(sizeof *p);
    *p = 42;
    free(p);
    return *p + n - 1; /* read after free */
}

static int double_free(int n)
{
    char *p = malloc(16);
    free(p);
    if (n > 0) {
        free(p);
    }
    return 0;
}

static int leak(int n)
{
    char *p = malloc(64);
    memset(p, 0, 64);
    return p[0] + n - 1; /* the pointer is lost when the function returns */
}

static int uninitialized(int n)
{
    int value;
    if (n > 5) {
        value = 1;
    }
    return value == 1 ? 10 : 20; /* branch on a value that was never written */
}

static int signed_overflow(int n)
{
    int big = INT_MAX;
    return big + n; /* overflow of a signed integer: undefined behavior */
}

static int shift_too_far(int n)
{
    return 1 << (31 + n); /* a shift by 32 is undefined */
}

static int null_dereference(int n)
{
    int *p = n > 0 ? NULL : &table[0];
    return *p;
}

struct entry {
    const char *name;
    int (*fn)(int);
};

static const struct entry cases[] = {
    {"heap_overflow", heap_overflow},     {"stack_overflow", stack_overflow},
    {"global_overflow", global_overflow}, {"use_after_free", use_after_free},
    {"double_free", double_free},         {"leak", leak},
    {"uninitialized", uninitialized},     {"signed_overflow", signed_overflow},
    {"shift_too_far", shift_too_far},     {"null_dereference", null_dereference},
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
            puts(cases[i].name);
        }
        return 0;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            int result = cases[i].fn(1);
            printf("%s: returned %d\n", cases[i].name, result);
            return 0;
        }
    }
    fprintf(stderr, "unknown case\n");
    return 2;
}
