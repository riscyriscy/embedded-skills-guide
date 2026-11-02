#ifndef BQUEUE_H
#define BQUEUE_H

#include <pthread.h>
#include <stdbool.h>

/* A queue of integers with a fixed capacity. Push waits while it is full and pop waits while it is
   empty. After bqueue_close, pop returns false once the queue is empty. */
#define BQUEUE_CAPACITY 4

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int item[BQUEUE_CAPACITY];
    unsigned head, count;
    bool closed;
} bqueue_t;

void bqueue_init(bqueue_t *q);
void bqueue_push(bqueue_t *q, int value);
bool bqueue_pop(bqueue_t *q, int *value);
void bqueue_close(bqueue_t *q);

#endif
