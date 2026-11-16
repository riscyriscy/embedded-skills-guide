/* The configuration of the other FreeRTOS examples, with one change: a failed assertion records the
   crash and resets the processor. */
#include "../FreeRTOSConfig.h"

#undef configASSERT
void crash_system(const char *condition, const char *file, unsigned line) __attribute__((noreturn));
#define configASSERT(x)                                                                            \
    do {                                                                                           \
        if ((x) == 0) {                                                                            \
            crash_system(#x, __FILE__, (unsigned)__LINE__);                                        \
        }                                                                                          \
    } while (0)

/* The crash path reads the task control blocks, so the kernel compiles it in (see
   freertos_tasks_c_additions.h) and records the highest address of each stack. */
#define configINCLUDE_FREERTOS_TASK_C_ADDITIONS_H 1
#define configRECORD_STACK_HIGH_ADDRESS           1
