#ifndef SENSOR_RINGBUF_H
#define SENSOR_RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* RINGBUF_CAPACITY comes from the build system (target_compile_definitions). */
#ifndef RINGBUF_CAPACITY
#error "RINGBUF_CAPACITY must be defined by the build"
#endif

typedef struct {
    int16_t data[RINGBUF_CAPACITY];
    size_t head;   /* index of the next write */
    size_t count;  /* samples stored, at most RINGBUF_CAPACITY */
} ringbuf_t;

void ringbuf_init(ringbuf_t *rb);
bool ringbuf_push(ringbuf_t *rb, int16_t sample);  /* true when the oldest sample was overwritten */
size_t ringbuf_count(const ringbuf_t *rb);
bool ringbuf_get(const ringbuf_t *rb, size_t index, int16_t *out);  /* index 0 is the oldest */

#endif
