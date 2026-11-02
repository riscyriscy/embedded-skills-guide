#include "fake_adc.h"

#include "adc.h"

static uint16_t next_raw;
static bool next_ok = true;
static unsigned calls;

void fake_adc_set(uint16_t raw, bool ok)
{
    next_raw = raw;
    next_ok = ok;
    calls = 0;
}

unsigned fake_adc_calls(void)
{
    return calls;
}

bool adc_read(uint16_t *raw)
{
    calls++;
    if (!next_ok) {
        return false;
    }
    *raw = next_raw;
    return true;
}
