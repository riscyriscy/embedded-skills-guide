#ifndef SENSOR_STATS_H
#define SENSOR_STATS_H

#include "sensor/ringbuf.h"

int16_t stats_mean(const ringbuf_t *rb);  /* 0 for an empty buffer */

#endif
