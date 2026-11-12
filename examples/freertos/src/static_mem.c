#include "FreeRTOS.h"
#include "task.h"

/* With configSUPPORT_STATIC_ALLOCATION set, the kernel asks the application for the memory of the
   idle task and of the timer task. The memory is two arrays and two task control blocks. */

#if configSUPPORT_STATIC_ALLOCATION == 1

void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *size)
{
    static StaticTask_t idle_tcb;
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
    *tcb = &idle_tcb;
    *stack = idle_stack;
    *size = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *size)
{
    static StaticTask_t timer_tcb;
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];
    *tcb = &timer_tcb;
    *stack = timer_stack;
    *size = configTIMER_TASK_STACK_DEPTH;
}

#endif
