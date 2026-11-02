#ifndef SENSOR_RINGBUF_H
#define SENSOR_RINGBUF_H

/**
 * @file ringbuf.h
 * @brief Fixed-capacity ring buffer of 16-bit samples.
 *
 * The buffer never allocates. When it is full, a new sample replaces the oldest one.
 * The functions are not thread-safe: one writer and one reader need their own locking.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* RINGBUF_CAPACITY comes from the build system (target_compile_definitions). */
#ifndef RINGBUF_CAPACITY
#error "RINGBUF_CAPACITY must be defined by the build"
#endif

/** A ring buffer. Treat the members as private and use the functions below. */
typedef struct {
    int16_t data[RINGBUF_CAPACITY]; /**< storage, written in a circle */
    size_t head;                    /**< index of the next write */
    size_t count;                   /**< samples stored, at most RINGBUF_CAPACITY */
} ringbuf_t;

/**
 * @brief Empties the buffer.
 * @param rb Buffer to initialise. Must not be NULL.
 */
void ringbuf_init(ringbuf_t *rb);

/**
 * @brief Adds a sample at the head.
 * @param rb     Buffer to write to. Must not be NULL.
 * @param sample Value to store.
 * @return true when the buffer was full and the oldest sample was overwritten.
 */
bool ringbuf_push(ringbuf_t *rb, int16_t sample);

/**
 * @brief Gives the number of samples stored.
 * @param rb Buffer to read. Must not be NULL.
 * @return A value from 0 to RINGBUF_CAPACITY.
 */
size_t ringbuf_count(const ringbuf_t *rb);

/**
 * @brief Reads one sample without removing it.
 * @param rb    Buffer to read. Must not be NULL.
 * @param index Position counted from the oldest sample, which is index 0.
 * @param out   Receives the sample. Must not be NULL.
 * @return true on success, false when index is not below ringbuf_count().
 */
bool ringbuf_get(const ringbuf_t *rb, size_t index, int16_t *out);

#endif
