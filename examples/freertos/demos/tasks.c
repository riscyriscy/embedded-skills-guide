#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* A worker that never blocks and a sampler that wakes every 50 ms. The sampler has the higher
   priority, so it runs at every wake up, whatever the worker does. */

static volatile uint32_t worker_loops;

static void worker_task(void *arg)
{
    (void)arg;
    for (;;) {
        worker_loops++;
    }
}

static void sampler_task(void *arg)
{
    (void)arg;
    TickType_t wake = xTaskGetTickCount();
    for (uint32_t i = 1; i <= 5; i++) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(50));
        rt_say_u32("sampler runs, sample", i);
    }
    rt_say_u32("worker loops done between the samples (nonzero)", worker_loops != 0u);
    rt_say_u32("free heap in bytes", (uint32_t)xPortGetFreeHeapSize());
    rt_finish(0);
}

int main(void)
{
    (void)xTaskCreate(worker_task, "worker", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTaskCreate(sampler_task, "sampler", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
