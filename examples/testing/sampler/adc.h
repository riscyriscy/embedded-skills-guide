#ifndef ADC_H
#define ADC_H

#include <stdbool.h>
#include <stdint.h>

/* Starts a conversion and returns the 12 bit result in *raw. False when the converter reports an
   error. On the target this function reads a peripheral, so unit tests replace it. */
bool adc_read(uint16_t *raw);

#endif
