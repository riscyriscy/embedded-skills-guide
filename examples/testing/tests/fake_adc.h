#ifndef FAKE_ADC_H
#define FAKE_ADC_H

#include <stdbool.h>
#include <stdint.h>

/* The next conversion returns raw and ok. A fake has working behaviour of its own, and a test sets
 * it up. */
void fake_adc_set(uint16_t raw, bool ok);
unsigned fake_adc_calls(void);

#endif
