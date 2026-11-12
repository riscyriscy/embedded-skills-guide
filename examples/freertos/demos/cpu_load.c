#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* The run time of each task, from the run time statistics of the kernel. The kernel of config_x
   counts the microseconds that each task runs. The worker takes 3 ms of every 10 ms, the logger 1
   ms of every 10 ms, and the monitor reads the counters after 200 ms. The rest of the time belongs
   to the idle task. */

#define MAX_TASKS 8u

static void periodic_work(uint32_t microseconds, uint32_t period_ms)
{
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        rt_busy_us(microseconds);
        (void)xTaskDelayUntil(&last, pdMS_TO_TICKS(period_ms));
    }
}

static void worker_task(void *arg)
{
    (void)arg;
    periodic_work(3000u, 10u);
}

static void logger_task(void *arg)
{
    (void)arg;
    periodic_work(1000u, 10u);
}

static void monitor_task(void *arg)
{
    static TaskStatus_t status[MAX_TASKS];
    char label[32];
    (void)arg;

    vTaskDelay(pdMS_TO_TICKS(200));
    uint32_t total = 0;
    UBaseType_t n = uxTaskGetSystemState(status, MAX_TASKS, &total);
    rt_say_u32("tasks reported", (uint32_t)n);
    rt_say_u32("total run time counter, microseconds", total);
    for (UBaseType_t i = 0; i < n; i++) {
        (void)strcpy(label, status[i].pcTaskName);
        (void)strcat(label, ", microseconds");
        rt_say_u32(label, status[i].ulRunTimeCounter);
        (void)strcpy(label, status[i].pcTaskName);
        (void)strcat(label, ", percent of the total");
        rt_say_u32(label, status[i].ulRunTimeCounter / (total / 100u));
    }
    rt_finish(0);
}

int main(void)
{
    (void)xTaskCreate(worker_task, "worker", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    (void)xTaskCreate(logger_task, "logger", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTaskCreate(monitor_task, "monitor", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
