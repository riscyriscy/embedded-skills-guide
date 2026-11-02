#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* A calibration table with a lookup that returns NULL for a name it does not know. */
struct channel {
    const char *name;
    int16_t offset;
    int16_t gain_permille;
};

static const struct channel channels[] = {
    {"temperature", -40, 1000},
    {"humidity", 5, 980},
};

static const struct channel *find_channel(const char *name)
{
    for (size_t i = 0; i < sizeof channels / sizeof channels[0]; i++) {
        if (strcmp(channels[i].name, name) == 0) {
            return &channels[i];
        }
    }
    return NULL;
}

static int32_t calibrate(const char *name, int16_t raw)
{
    const struct channel *ch = find_channel(name);
    return ((int32_t)raw * ch->gain_permille) / 1000 + ch->offset;
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        printf("%s: %d\n", argv[i], (int)calibrate(argv[i], 1000));
    }
    return 0;
}
