#ifndef FREERTOS_CONFIG_X_H
#define FREERTOS_CONFIG_X_H

/* The configuration of the programs that need run time statistics, the trace facility or static
   allocation. It is the configuration of the other programs with four settings changed and two
   macros defined. A kernel with these options is a different build of the kernel, so these
   programs link the library freertos_x. */

#include "../FreeRTOSConfig.h"

/* Every task has two notification slots, index 0 for the application and index 1 for a library. */
#undef configTASK_NOTIFICATION_ARRAY_ENTRIES
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 2

/* The application supplies the memory of the idle task and the timer task: see src/static_mem.c. */
#undef configSUPPORT_STATIC_ALLOCATION
#define configSUPPORT_STATIC_ALLOCATION 1

/* The trace facility adds the task state functions, and the run time counter adds the time that
   each task has run. The counter is the microsecond clock of rt.c. */
#undef configUSE_TRACE_FACILITY
#define configUSE_TRACE_FACILITY 1
#undef configGENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS 1
extern uint32_t rt_now_us(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ((void)0)
#define portGET_RUN_TIME_COUNTER_VALUE()         rt_now_us()

#endif
