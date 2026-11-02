#include <pthread.h>
#include <stdio.h>

/* Two threads add to one counter. Without a lock the additions overlap and some are lost. */

#define ITERATIONS 100000

static long counter;
#ifdef USE_LOCK
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
#ifdef USE_LOCK
        pthread_mutex_lock(&lock);
#endif
        counter++;
#ifdef USE_LOCK
        pthread_mutex_unlock(&lock);
#endif
    }
    return NULL;
}

int main(void)
{
    pthread_t a;
    pthread_t b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("counter = %ld (expected %d)\n", counter, 2 * ITERATIONS);
    return counter == 2 * ITERATIONS ? 0 : 1;
}
