/* Compiled into tasks.c, the only place that may read the task control blocks. The crash path calls
   crash_each_task, which shows every task to a function without calling the kernel: the lists are
   read as plain memory, every pointer is checked, and the number of tasks is bounded, because the
   crash may have damaged the lists. */
#include "crash.h"

extern const char _ram_begin[], _ram_end[];

static int crash_in_ram(const void *p)
{
    uint32_t a = (uint32_t)p;
    return a >= (uint32_t)_ram_begin && a < (uint32_t)_ram_end && (a & 3u) == 0u;
}

static void crash_visit_list(const List_t *list, crash_task_visit visit, unsigned *budget)
{
    if (!crash_in_ram(list)) {
        return;
    }
    const ListItem_t *end = (const ListItem_t *)&list->xListEnd;
    const ListItem_t *item = list->xListEnd.pxNext;
    for (unsigned n = list->uxNumberOfItems; n > 0u && item != end && *budget > 0u; n--) {
        if (!crash_in_ram(item)) {
            return;
        }
        const TCB_t *tcb = item->pvOwner;
        if (!crash_in_ram(tcb)) {
            return;
        }
        visit(tcb->pcTaskName, tcb->pxStack, tcb->pxTopOfStack, tcb->pxEndOfStack,
              tcb == pxCurrentTCB);
        (*budget)--;
        item = item->pxNext;
    }
}

void crash_each_task(crash_task_visit visit)
{
    unsigned budget = 16;
    for (unsigned i = 0; i < configMAX_PRIORITIES; i++) {
        crash_visit_list(&pxReadyTasksLists[i], visit, &budget);
    }
    crash_visit_list(pxDelayedTaskList, visit, &budget);
    crash_visit_list(pxOverflowDelayedTaskList, visit, &budget);
    crash_visit_list(&xPendingReadyList, visit, &budget);
    crash_visit_list(&xSuspendedTaskList, visit, &budget);
}
