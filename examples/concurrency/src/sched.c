#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

/* The real time part of POSIX threads: a mutex that passes priority to its holder (the protocol of
   Chapter 17) and a thread with the fixed priority policy SCHED_FIFO. */

static void *nothing(void *arg)
{
    return arg;
}

int main(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    int rc = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    printf("priority inheritance protocol for a mutex: %s\n", rc == 0 ? "accepted" : strerror(rc));
    pthread_mutex_t lock;
    rc = pthread_mutex_init(&lock, &attr);
    printf("mutex with that protocol: %s\n", rc == 0 ? "created" : strerror(rc));

    printf("priorities of SCHED_FIFO: %d to %d\n", sched_get_priority_min(SCHED_FIFO),
           sched_get_priority_max(SCHED_FIFO));
    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);
    struct sched_param param = {.sched_priority = 20};
    pthread_attr_setschedparam(&tattr, &param);
    pthread_t t;
    rc = pthread_create(&t, &tattr, nothing, NULL);
    if (rc == 0) {
        pthread_join(t, NULL);
        printf("thread with SCHED_FIFO priority 20: created\n");
    } else {
        printf("thread with SCHED_FIFO priority 20: %s\n", strerror(rc));
    }
    return 0;
}
