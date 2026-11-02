#include <pthread.h>
#include <stdio.h>

#include "bqueue.h"

/* Two producers push 5000 numbers each, three consumers pop them. The queue holds only 4 items, so
   both sides wait often. The sum of what was popped must equal the sum of what was pushed. */

#define PER_PRODUCER 5000

static bqueue_t queue;
static long popped_sum[3];
static long popped_count[3];

static void *producer(void *arg)
{
    int base = *(int *)arg;
    for (int i = 1; i <= PER_PRODUCER; i++) {
        bqueue_push(&queue, base + i);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int id = *(int *)arg;
    int value;
    while (bqueue_pop(&queue, &value)) {
        popped_sum[id] += value;
        popped_count[id]++;
    }
    return NULL;
}

int main(void)
{
    bqueue_init(&queue);
    int bases[2] = {0, 100000};
    int ids[3] = {0, 1, 2};
    pthread_t p[2], c[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&c[i], NULL, consumer, &ids[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_create(&p[i], NULL, producer, &bases[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(p[i], NULL);
    }
    bqueue_close(&queue);
    long sum = 0, count = 0;
    for (int i = 0; i < 3; i++) {
        pthread_join(c[i], NULL);
        sum += popped_sum[i];
        count += popped_count[i];
    }
    long expected = 0;
    for (int b = 0; b < 2; b++) {
        for (int i = 1; i <= PER_PRODUCER; i++) {
            expected += bases[b] + i;
        }
    }
    printf("popped %ld items with sum %ld, expected %d items with sum %ld\n", count, sum,
           2 * PER_PRODUCER, expected);
    return (sum == expected && count == 2 * PER_PRODUCER) ? 0 : 1;
}
