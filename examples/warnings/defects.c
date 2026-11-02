#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum color { RED, GREEN, BLUE };

int16_t scale(int32_t raw)
{
    return raw * 3;
}

uint8_t checksum(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

const char *name(enum color c)
{
    switch (c) {
    case RED:
        return "red";
    case GREEN:
        return "green";
    }
    return "?";
}

void log_value(int value)
{
    printf("value: %s\n", value);
}

int shadowed(int x)
{
    int result = 1;
    {
        int result = x * 2;
        (void)result;
    }
    return result;
}

void copy_name(void)
{
    char buffer[4];
    strcpy(buffer, "overflow");
    puts(buffer);
}

int read_register(void);

int maybe_set(int flag)
{
    int value;
    if (flag) {
        value = read_register();
    }
    return value + 1;
}

float half(float x)
{
    return x * 0.5;
}

int never_true(unsigned int u)
{
    return u < 0;
}

int fall_through(int a)
{
    switch (a) {
    case 1:
        a++;
    case 2:
        a += 2;
        break;
    default:
        break;
    }
    return a;
}

long *widen(char *p)
{
    return (long *)p;
}

int unused_parameter(int used, int unused)
{
    return used;
}
