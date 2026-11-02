#ifndef SAMPLER_H
#define SAMPLER_H

#include <stdbool.h>

#include "sensor/ringbuf.h"

typedef struct {
    ringbuf_t samples; /* millivolts */
    unsigned errors;   /* conversions that failed */
} sampler_t;

void sampler_init(sampler_t *s);

/* Reads the converter once and stores the result in millivolts for a 3.3 V reference. Returns
   false, and stores nothing, when the converter reports an error. */
bool sampler_poll(sampler_t *s);

#endif
