/* The first program of the test image: loads the module, reads the device twice,
   removes the module and prints the kernel messages of the module. Statically linked,
   so the image needs no C library files. */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/klog.h>
#include <unistd.h>

static void read_device(void)
{
    char text[32] = "";
    int fd = open("/dev/sfw_counter", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/sfw_counter");
        return;
    }
    ssize_t n = read(fd, text, sizeof(text) - 1);
    printf("read %zd bytes: %s", n, n > 0 ? text : "\n");
    close(fd);
}

static void show_log(void)
{
    static char log[8192];
    int len = klogctl(3, log, sizeof(log) - 1); /* SYSLOG_ACTION_READ_ALL */
    if (len > 0) {
        log[len] = '\0';
        for (char *line = strtok(log, "\n"); line != NULL;
             line = strtok(NULL, "\n")) {
            if (strstr(line, "sfw_counter") != NULL) {
                puts(line);
            }
        }
    }
}

/* The read runs in a child process: a kernel oops kills the process that caused it,
   and init must survive. */
static void read_in_child(void)
{
    pid_t pid = fork();
    if (pid == 0) {
        read_device();
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("the reading process was killed by signal %d\n", WTERMSIG(status));
    }
}

int main(void)
{
    mount("proc", "/proc", "proc", 0, "");
    mount("devtmpfs", "/dev", "devtmpfs", 0, ""); /* /dev/sfw_counter appears here */
    int fd = open("/sfw_counter.ko", O_RDONLY);
    if (fd < 0) {
        perror("open module");
        return 1;
    }
    /* the third argument is the parameter string, as on the insmod command line */
    char params[64] = "start=100";
    int pf = open("/params", O_RDONLY); /* the script may add module parameters */
    int single = pf >= 0; /* with parameters, read once: a failed read keeps the lock */
    if (pf >= 0) {
        ssize_t got = read(pf, params, sizeof(params) - 1);
        params[got > 0 ? got : 0] = '\0';
        close(pf);
    }
    if (syscall(SYS_finit_module, fd, params, 0) != 0) {
        perror("finit_module");
        show_log();
        sync();
        reboot(RB_POWER_OFF);
        return 1;
    }
    close(fd);
    puts("module loaded");
    read_in_child();
    if (single) {
        show_log();
        sync();
        reboot(RB_POWER_OFF);
        return 0;
    }
    read_in_child();
    if (syscall(SYS_delete_module, "sfw_counter", 0) != 0) {
        perror("delete_module");
    }
    puts("module removed");
    read_in_child(); /* the device is gone, so this open must fail */

    show_log();
    sync();
    reboot(RB_POWER_OFF);
    return 0;
}
