#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

/* Four threads each add a quarter of an array. pthread_join returns their results. */

#define COUNT   1000000u
#define THREADS 4u

static uint32_t data[COUNT];

typedef struct {
    unsigned first, last;
    uint64_t sum;
} part_t;

static void *add_part(void *arg)
{
    part_t *part = arg;
    uint64_t sum = 0;
    for (unsigned i = part->first; i < part->last; i++) {
        sum += data[i];
    }
    part->sum = sum;
    return part;
}

int main(void)
{
    for (unsigned i = 0; i < COUNT; i++) {
        data[i] = i % 1000u;
    }
    pthread_t thread[THREADS];
    part_t part[THREADS];
    for (unsigned t = 0; t < THREADS; t++) {
        part[t].first = t * (COUNT / THREADS);
        part[t].last = (t + 1u) * (COUNT / THREADS);
        if (pthread_create(&thread[t], NULL, add_part, &part[t]) != 0) {
            return 1;
        }
    }
    uint64_t total = 0;
    for (unsigned t = 0; t < THREADS; t++) {
        void *result;
        (void)pthread_join(thread[t], &result);
        total += ((part_t *)result)->sum;
        printf("thread %u: samples %u to %u, sum %llu\n", t, part[t].first, part[t].last - 1u,
               (unsigned long long)part[t].sum);
    }
    printf("total %llu\n", (unsigned long long)total);
    return 0;
}
