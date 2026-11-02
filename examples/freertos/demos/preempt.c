#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* The two jobs of demos/superloop.c as two tasks. The fast task has the higher priority. */

static void fast_task(void *arg)
{
    (void)arg;
    TickType_t wake = xTaskGetTickCount();
    uint32_t runs = 0, worst = 0;
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(10)); /* wake is the release time of the next job */
    while (wake <= 1000u) {
        uint32_t late = rt_now_us() - (uint32_t)wake * 1000u;
        if (late > worst) {
            worst = late;
        }
        rt_busy_us(200);
        runs++;
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
    rt_say_u32("fast job runs in one second", runs);
    rt_say_u32("worst delay of the start of the fast job, microseconds", worst);
    rt_finish(0);
}

static void slow_task(void *arg)
{
    (void)arg;
    TickType_t wake = xTaskGetTickCount();
    vTaskDelay(pdMS_TO_TICKS(5));
    for (;;) {
        rt_busy_us(30000);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
    }
}

int main(void)
{
    (void)xTaskCreate(fast_task, "fast", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    (void)xTaskCreate(slow_task, "slow", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
