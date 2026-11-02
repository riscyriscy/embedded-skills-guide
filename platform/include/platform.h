#ifndef PLATFORM_H
#define PLATFORM_H

void plat_puts(const char *text); /* write text and a newline to the console */
void plat_exit(int status) __attribute__((noreturn));

#endif
