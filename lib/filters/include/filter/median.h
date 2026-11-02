#ifndef FILTER_MEDIAN_H
#define FILTER_MEDIAN_H

#include "sensor/ringbuf.h"

/* Median of the three newest samples, which rejects a single spike. 0 when fewer than 3 samples. */
int16_t filter_median3(const ringbuf_t *rb);

#endif
