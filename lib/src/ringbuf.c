#include "sensor/ringbuf.h"

void ringbuf_init(ringbuf_t *rb)
{
    rb->head = 0;
    rb->count = 0;
}

bool ringbuf_push(ringbuf_t *rb, int16_t sample)
{
    bool overwrote = rb->count == RINGBUF_CAPACITY;

    rb->data[rb->head] = sample;
    rb->head = (rb->head + 1u) % RINGBUF_CAPACITY;
    if (!overwrote) {
        rb->count++;
    }
    return overwrote;
}

size_t ringbuf_count(const ringbuf_t *rb)
{
    return rb->count;
}

bool ringbuf_get(const ringbuf_t *rb, size_t index, int16_t *out)
{
    if (index >= rb->count) {
        return false;
    }
    size_t oldest = (rb->head + RINGBUF_CAPACITY - rb->count) % RINGBUF_CAPACITY;
    *out = rb->data[(oldest + index) % RINGBUF_CAPACITY];
    return true;
}
