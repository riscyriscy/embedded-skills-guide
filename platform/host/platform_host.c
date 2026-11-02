#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

void plat_puts(const char *text)
{
    puts(text);
}

void plat_exit(int status)
{
    exit(status);
}
