#include <stddef.h>
#include <stdint.h>

#include "print.h"

/* The same CRC in C: the table is built at start-up, in RAM, by code that runs on every boot. */
static uint16_t crc16_table[256];

static void crc16_init(void)
{
    for (unsigned i = 0; i < 256u; i++) {
        uint16_t crc = (uint16_t)i;
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1u) != 0u ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
        }
        crc16_table[i] = crc;
    }
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc = (uint16_t)((crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFFu]);
    }
    return crc;
}

int main(void)
{
    static const uint8_t check[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    crc16_init();
    print_u32("crc16 of 123456789", crc16(check, sizeof check));
    plat_exit(0);
}
