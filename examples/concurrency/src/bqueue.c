#include "bqueue.h"

void bqueue_init(bqueue_t *q)
{
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    q->head = 0;
    q->count = 0;
    q->closed = false;
}

void bqueue_push(bqueue_t *q, int value)
{
    pthread_mutex_lock(&q->lock);
    /* A loop, not an if: a wait can return without a signal, and another thread may have taken the
       free slot between the signal and the return of the wait. */
    while (q->count == BQUEUE_CAPACITY) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    q->item[(q->head + q->count) % BQUEUE_CAPACITY] = value;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

bool bqueue_pop(bqueue_t *q, int *value)
{
    pthread_mutex_lock(&q->lock);
    while (q->count == 0 && !q->closed) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    if (q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }
    *value = q->item[q->head];
    q->head = (q->head + 1u) % BQUEUE_CAPACITY;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return true;
}

void bqueue_close(bqueue_t *q)
{
    pthread_mutex_lock(&q->lock);
    q->closed = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}
