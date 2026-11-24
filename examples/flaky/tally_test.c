#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* A test that passes when the two threads do not overlap and fails when they do. Each thread adds
   500 events to one tally, so a correct run ends at 1000. The option --locked takes a mutex around
   the addition, which is the repair. */

#define EVENTS 500

static long tally;
static int use_lock;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < EVENTS; i++) {
        if (use_lock) {
            pthread_mutex_lock(&lock);
        }
        tally++;
        if (use_lock) {
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    use_lock = argc > 1 && strcmp(argv[1], "--locked") == 0;

    pthread_t a;
    pthread_t b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("tally = %ld, expected %d\n", tally, 2 * EVENTS);
    return tally == 2 * EVENTS ? 0 : 1;
}
