#include "sampler.h"

#include "adc.h"

#define ADC_FULL_SCALE 4095u
#define REFERENCE_MV   3300u

void sampler_init(sampler_t *s)
{
    ringbuf_init(&s->samples);
    s->errors = 0;
}

bool sampler_poll(sampler_t *s)
{
    uint16_t raw = 0;
    if (!adc_read(&raw)) {
        s->errors++;
        return false;
    }
    uint32_t millivolts = ((uint32_t)raw * REFERENCE_MV) / ADC_FULL_SCALE;
    (void)ringbuf_push(&s->samples, (int16_t)millivolts);
    return true;
}
