#define _GNU_SOURCE /* for pthread_getattr_np */
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* The stack of a thread: the default size, the least size, and a thread with a stack of 64 KiB. */

static void *report(void *arg)
{
    size_t size = 0;
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);
    pthread_attr_getstacksize(&attr, &size);
    printf("%s: stack of %zu bytes\n", (const char *)arg, size);
    pthread_attr_destroy(&attr);
    return NULL;
}

int main(void)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t size = 0;
    pthread_attr_getstacksize(&attr, &size);
    printf("default stack size of a new thread: %zu bytes\n", size);
    printf("PTHREAD_STACK_MIN: %ld bytes\n", (long)PTHREAD_STACK_MIN);

    pthread_t t;
    pthread_create(&t, NULL, report, "thread with the default stack");
    pthread_join(t, NULL);

    pthread_attr_setstacksize(&attr, 64u * 1024u);
    pthread_create(&t, &attr, report, "thread with 64 KiB");
    pthread_join(t, NULL);

    int rc = pthread_attr_setstacksize(&attr, 1024);
    printf("a stack of 1024 bytes: %s\n", rc == 0 ? "accepted" : strerror(rc));
    return 0;
}
