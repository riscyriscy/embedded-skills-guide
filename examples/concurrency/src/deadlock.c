#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Two threads need the locks A and B. Run with "abba" and the threads take them in opposite orders;
   a barrier makes sure that each holds one before it asks for the other. Run with "ordered" and
   both take them in the order of their addresses. Run with "sequential" and the threads run one
   after the other, in opposite orders: nothing can hang, but the order of the locks is still
   inconsistent. A timed lock turns the deadlock into an error: thread one gives up after 300 ms and
   thread two after 1000 ms, so the lock that thread one lets go lets thread two finish. */

static pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t both_hold_one;
static int ordered;
static int sequential;
static int timeouts;
#define WAIT_ONE_MS 300
#define WAIT_TWO_MS 1000
static char report[2][160]; /* each thread writes its own line, main prints them in order */

static int lock_for(pthread_mutex_t *m, long milliseconds)
{
    struct timespec limit;
    clock_gettime(CLOCK_REALTIME, &limit);
    limit.tv_sec += milliseconds / 1000;
    limit.tv_nsec += (milliseconds % 1000) * 1000000L;
    if (limit.tv_nsec >= 1000000000L) {
        limit.tv_sec++;
        limit.tv_nsec -= 1000000000L;
    }
    return pthread_mutex_timedlock(m, &limit);
}

static void *take_both(void *arg)
{
    const char *name = arg;
    char *out = report[strcmp(name, "one") == 0 ? 0 : 1];
    size_t used = 0;
    pthread_mutex_t *first = (strcmp(name, "one") == 0) ? &lock_a : &lock_b;
    pthread_mutex_t *second = (first == &lock_a) ? &lock_b : &lock_a;
    if (ordered && (uintptr_t)first > (uintptr_t)second) {
        pthread_mutex_t *swap = first;
        first = second;
        second = swap;
    }
    pthread_mutex_lock(first);
    used += (size_t)snprintf(out + used, sizeof report[0] - used, "thread %s holds lock %c; ", name,
                             first == &lock_a ? 'A' : 'B');
    if (!ordered && !sequential) {
        pthread_barrier_wait(&both_hold_one);
    }
    long wait = (strcmp(name, "one") == 0) ? WAIT_ONE_MS : WAIT_TWO_MS;
    int rc = sequential ? pthread_mutex_lock(second) : lock_for(second, wait);
    if (rc == ETIMEDOUT) {
        snprintf(out + used, sizeof report[0] - used,
                 "waited %ld ms for lock %c and gave up (ETIMEDOUT)", wait,
                 second == &lock_a ? 'A' : 'B');
        timeouts++;
    } else {
        snprintf(out + used, sizeof report[0] - used, "has both locks");
        pthread_mutex_unlock(second);
    }
    pthread_mutex_unlock(first);
    return NULL;
}

int main(int argc, char **argv)
{
    ordered = (argc > 1 && strcmp(argv[1], "ordered") == 0);
    sequential = (argc > 1 && strcmp(argv[1], "sequential") == 0);
    pthread_barrier_init(&both_hold_one, NULL, 2);
    pthread_t one, two;
    pthread_create(&one, NULL, take_both, "one");
    if (sequential) {
        pthread_join(one, NULL);
    }
    pthread_create(&two, NULL, take_both, "two");
    if (!sequential) {
        pthread_join(one, NULL);
    }
    pthread_join(two, NULL);
    printf("%s\n%s\n", report[0], report[1]);
    printf("%s: %d thread(s) gave up\n", ordered ? "ordered" : (sequential ? "sequential" : "abba"),
           timeouts);
    return 0;
}
