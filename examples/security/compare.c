/* Two ways to compare a 16 byte authentication tag. leaky_equal stops at the first
   difference, so the time it takes tells an attacker how many leading bytes were
   right. ct_equal always looks at every byte. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG 16u

__attribute__((noinline)) bool leaky_equal(const uint8_t *a, const uint8_t *b,
                                           size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

__attribute__((noinline)) bool ct_equal(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t difference = 0;
    for (size_t i = 0; i < n; i++) {
        difference |= (uint8_t)(a[i] ^ b[i]);
    }
    return difference == 0u;
}

int main(int argc, char **argv)
{
    /* argv[1]: "leaky" or "ct". argv[2]: the index of the byte that differs. */
    if (argc != 3) {
        return 2;
    }
    uint8_t expected[TAG], received[TAG];
    for (size_t i = 0; i < TAG; i++) {
        expected[i] = received[i] = (uint8_t)(0xA0u + i);
    }
    received[(size_t)strtoul(argv[2], NULL, 10) % TAG] ^= 0x01u;
    bool same = strcmp(argv[1], "ct") == 0 ? ct_equal(expected, received, TAG)
                                           : leaky_equal(expected, received, TAG);
    printf("%s, difference at byte %s: %s\n", argv[1], argv[2],
           same ? "equal" : "different");
    return 0;
}
