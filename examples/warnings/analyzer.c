#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void leak(void)
{
    char *p = malloc(16);
    if (p == NULL) {
        return;
    }
    p[0] = 'x';
}

void use_after_free(void)
{
    char *p = malloc(8);
    if (p == NULL) {
        return;
    }
    free(p);
    p[0] = 'x';
}

void double_free(void)
{
    char *p = malloc(8);
    free(p);
    free(p);
}

int null_after_use(int *p)
{
    int first = *p;
    if (p == NULL) {
        return -1;
    }
    return first;
}

void file_leak(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return;
    }
    char line[32];
    if (fgets(line, sizeof line, f) == NULL) {
        return;
    }
    fclose(f);
}
