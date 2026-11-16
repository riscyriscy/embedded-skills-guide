#ifndef CRASH_H
#define CRASH_H

#include <stdint.h>

/* The crash record: a small block of RAM at a fixed address that survives a reset. The bootloader
   and the application include this header and link crash.c, so both see the same layout. */

enum reset_class {
    RESET_POWER_ON,  /* no valid record: power was lost, or the first start */
    RESET_UNPLANNED, /* the record says "running": a watchdog, a lock-up or a brown-out */
    RESET_REQUESTED, /* the firmware asked for the reset and gave a code */
    RESET_CRASH,     /* an assertion, a fault or a kernel hook: the record holds text */
};

/* Classifies the last reset without changing the record. The bootloader calls this. */
enum reset_class crash_peek(void);

/* Classifies the last reset, counts it and marks the record "running". Call it once, first thing in
   main of the application. The text of a crash stays readable until the next crash. */
enum reset_class crash_start(void);

unsigned crash_boots(void);         /* resets since the last power-on, counting this one */
unsigned crash_count(void);         /* crashes since the last power-on */
uint32_t crash_code(void);          /* the code given to crash_request_reset */
unsigned crash_lines(void);         /* number of lines of crash text, 0 when there is none */
const char *crash_line(unsigned i); /* line i of the crash text */

void crash_request_reset(uint32_t code) __attribute__((noreturn));

/* Records what failed, the task and some code addresses from the stack, then resets. */
void crash_system(const char *condition, const char *file, unsigned line) __attribute__((noreturn));

const char *crash_class_name(enum reset_class c);

/* Called by crash_each_task for every task: the name, the lowest address of the stack, the saved
   stack pointer, the highest address of the stack and whether the task was running. The kernel side
   is freertos_tasks_c_additions.h. */
typedef void (*crash_task_visit)(const char *name, const uint32_t *base, const uint32_t *top,
                                 const uint32_t *end, int running);
void crash_each_task(crash_task_visit visit);

#define CRASH_IF(x)                                                                                \
    do {                                                                                           \
        if (x) {                                                                                   \
            crash_system(#x, __FILE__, (unsigned)__LINE__);                                        \
        }                                                                                          \
    } while (0)

#endif
