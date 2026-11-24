/* Reads the record of the unit and uses it. The same image runs on every unit, and this is the only
   thing that makes one unit different from the next. */
#include <stdint.h>

#include "platform.h"
#include "unit_record.h"

extern const unit_record_t prov_record; /* placed at the last page of the flash by link_prov.ld */

static char *put(char *out, const char *text)
{
    while (*text != '\0') {
        *out++ = *text++;
    }
    return out;
}

static char *put_number(char *out, int32_t value)
{
    char digits[11];
    unsigned n = 0;
    uint32_t magnitude = value < 0 ? 0u - (uint32_t)value : (uint32_t)value;
    do {
        digits[n++] = (char)('0' + magnitude % 10u);
        magnitude /= 10u;
    } while (magnitude != 0u);
    if (value < 0) {
        *out++ = '-';
    }
    while (n > 0u) {
        *out++ = digits[--n];
    }
    return out;
}

static char *put_hex(char *out, const uint8_t *bytes, unsigned count, char separator)
{
    static const char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < count; i++) {
        if (i > 0u && separator != '\0') {
            *out++ = separator;
        }
        *out++ = hex[bytes[i] >> 4];
        *out++ = hex[bytes[i] & 0xFu];
    }
    return out;
}

static void line(char *start, char *end)
{
    *end = '\0';
    plat_puts(start);
}

int main(void)
{
    char buf[80];
    char *p;

    unit_status_t status = unit_check(&prov_record);
    p = put(buf, "record: ");
    p = put(p, unit_status_name(status));
    line(buf, p);
    if (status != UNIT_OK) {
        plat_exit(1);
    }

    p = put(buf, "serial ");
    p = put(p, prov_record.serial);
    p = put(p, ", board revision ");
    p = put_number(p, prov_record.hw_rev);
    line(buf, p);

    p = put(buf, "mac ");
    p = put_hex(p, prov_record.mac, 6, ':');
    p = put(p, ", key id ");
    p = put_hex(p, prov_record.key_id, 8, '\0');
    line(buf, p);

    p = put(buf, "calibration: offset ");
    p = put_number(p, prov_record.adc_offset);
    p = put(p, " counts, gain ");
    p = put_number(p, (int32_t)prov_record.adc_gain_ppm);
    p = put(p, " ppm");
    line(buf, p);

    p = put(buf, "a raw reading of 2048 counts is ");
    p = put_number(p, unit_correct(&prov_record, 2048));
    line(buf, p);
    plat_exit(0);
}
