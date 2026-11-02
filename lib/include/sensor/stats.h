#ifndef SENSOR_STATS_H
#define SENSOR_STATS_H

/**
 * @file stats.h
 * @brief Statistics over the samples of a ring buffer.
 */

#include "sensor/ringbuf.h"

/**
 * @brief Computes the mean of the stored samples, rounded toward zero.
 * @param rb Buffer to read. Must not be NULL.
 * @return The mean, or 0 for an empty buffer.
 */
int16_t stats_mean(const ringbuf_t *rb);

#endif
