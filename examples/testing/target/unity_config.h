#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

/* Unity reads this file on the target because the build defines UNITY_INCLUDE_CONFIG_H. Floating
   point support is left out to keep the image small. Output goes through one function, which is
   defined in output_semihost.c or in output_uart.c. */
#define UNITY_EXCLUDE_FLOAT
#define UNITY_EXCLUDE_DOUBLE

void unity_putc(int c);
#define UNITY_OUTPUT_CHAR(c) unity_putc(c)

#endif
