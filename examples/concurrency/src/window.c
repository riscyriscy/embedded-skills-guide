#include <pthread.h>
#include <stdio.h>

#include "sensor/ringbuf.h"

/* One writer replaces the whole window of 16 samples with a new value, 20000 times. Two readers
   look at the window and expect to see one value in all 16 places. Build with -DUSE_LOCK for a
   reader writer lock around the window. */

#define ROUNDS 20000

static ringbuf_t window;
static volatile int done;
#ifdef USE_LOCK
static pthread_rwlock_t window_lock = PTHREAD_RWLOCK_INITIALIZER;
#endif
static unsigned torn[2];

static void *writer(void *arg)
{
    (void)arg;
    for (int round = 1; round <= ROUNDS; round++) {
#ifdef USE_LOCK
        pthread_rwlock_wrlock(&window_lock);
#endif
        for (int i = 0; i < 16; i++) {
            (void)ringbuf_push(&window, (int16_t)(round % 30000));
        }
#ifdef USE_LOCK
        pthread_rwlock_unlock(&window_lock);
#endif
    }
    done = 1;
    return NULL;
}

static void *reader(void *arg)
{
    unsigned *count = arg;
    while (!done) {
#ifdef USE_LOCK
        pthread_rwlock_rdlock(&window_lock);
#endif
        int16_t first = 0, other = 0;
        int same = 1;
        for (size_t i = 0; i < ringbuf_count(&window); i++) {
            (void)ringbuf_get(&window, i, &other);
            if (i == 0) {
                first = other;
            } else if (other != first) {
                same = 0;
            }
        }
#ifdef USE_LOCK
        pthread_rwlock_unlock(&window_lock);
#endif
        if (!same) {
            (*count)++;
        }
    }
    return NULL;
}

int main(void)
{
    ringbuf_init(&window);
    pthread_t w, r0, r1;
    pthread_create(&r0, NULL, reader, &torn[0]);
    pthread_create(&r1, NULL, reader, &torn[1]);
    pthread_create(&w, NULL, writer, NULL);
    pthread_join(w, NULL);
    pthread_join(r0, NULL);
    pthread_join(r1, NULL);
    printf("windows that held more than one value: reader 0 saw %u, reader 1 saw %u\n", torn[0],
           torn[1]);
    return torn[0] + torn[1] == 0 ? 0 : 1;
}
