#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "sensor/ringbuf.h"
#include "sensor/stats.h"

#ifndef SENSORD_VERSION
#define SENSORD_VERSION "unknown"
#endif

/* A service for the Linux image of Chapters 21 to 23. Once a second it takes a sample, pushes it
   into the ring buffer of the sensor library, and every fifth sample it logs the mean. It runs the
   way a service should run: in the foreground, with a log on standard output, with a clean stop on
   SIGTERM, with a re-read of its settings on SIGHUP, and with the two messages of systemd (READY
   and WATCHDOG) when it was started by systemd. */

static volatile sig_atomic_t stop_requested;
static volatile sig_atomic_t reload_requested;

static void on_signal(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        stop_requested = 1;
    } else if (sig == SIGHUP) {
        reload_requested = 1;
    }
}

static void say(const char *text)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    printf("[%5ld.%03ld] sensord[%d]: %s\n", (long)now.tv_sec, now.tv_nsec / 1000000L,
           (int)getpid(), text);
    fflush(stdout);
}

/* The protocol of sd_notify(3) is one datagram on the socket that systemd names in NOTIFY_SOCKET.
 */
static void notify(const char *state)
{
    const char *path = getenv("NOTIFY_SOCKET");
    if (path == NULL || path[0] == '\0') {
        return;
    }
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return;
    }
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    strncpy(address.sun_path, path, sizeof address.sun_path - 1);
    if (address.sun_path[0] == '@') {
        address.sun_path[0] = '\0'; /* an abstract socket */
    }
    socklen_t length = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(path));
    (void)sendto(fd, state, strlen(state), 0, (struct sockaddr *)&address, length);
    close(fd);
}

static int read_period(const char *file, int fallback)
{
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        return fallback;
    }
    int value = fallback;
    if (fscanf(f, "%d", &value) != 1 || value < 1 || value > 3600) {
        value = fallback;
    }
    fclose(f);
    return value;
}

int main(int argc, char **argv)
{
    const char *period_file = (argc > 1) ? argv[1] : "/etc/sensord.period";
    struct sigaction action = {.sa_handler = on_signal};
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    ringbuf_t rb;
    ringbuf_init(&rb);
    int period = read_period(period_file, 1);
    char line[96];
    snprintf(line, sizeof line, "version %s started, period %d s", SENSORD_VERSION, period);
    say(line);
    notify("READY=1");

    long counter = 0;
    while (!stop_requested) {
        if (reload_requested) {
            reload_requested = 0;
            period = read_period(period_file, period);
            snprintf(line, sizeof line, "reloaded, period %d s", period);
            say(line);
        }
        (void)ringbuf_push(&rb, (int16_t)(100 + (counter * 7) % 23));
        counter++;
        if (counter % 5 == 0) {
            snprintf(line, sizeof line, "%ld samples, mean of the last %zu is %d", counter,
                     ringbuf_count(&rb), stats_mean(&rb));
            say(line);
        }
        notify("WATCHDOG=1");
        struct timespec pause = {.tv_sec = period, .tv_nsec = 0};
        nanosleep(&pause, NULL); /* a signal ends the sleep early */
    }
    say("stopping");
    notify("STOPPING=1");
    return 0;
}
