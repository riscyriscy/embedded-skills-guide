#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

/* Four threads add to one counter 250000 times each. Build with -DUSE_MUTEX or -DUSE_ATOMIC for the
   two ways to keep the additions apart. */

#define ITERATIONS 250000
#define THREADS    4

#if defined(USE_ATOMIC)
static atomic_long counter;
#else
static long counter;
#endif
#ifdef USE_MUTEX
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
#if defined(USE_MUTEX)
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
#elif defined(USE_ATOMIC)
        atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
#else
        counter++;
#endif
    }
    return NULL;
}

int main(void)
{
    pthread_t thread[THREADS];
    for (int t = 0; t < THREADS; t++) {
        pthread_create(&thread[t], NULL, worker, NULL);
    }
    for (int t = 0; t < THREADS; t++) {
        pthread_join(thread[t], NULL);
    }
    long total = (long)counter;
    printf("counter = %ld, expected %d\n", total, THREADS * ITERATIONS);
    return total == THREADS * ITERATIONS ? 0 : 1;
}
