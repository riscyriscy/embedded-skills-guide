#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* A task that runs every 10 ms. Its work takes 3 ms, except in cycle 3, where it takes 24 ms. Then
   the wake time of the next two periods passes while the task works, and xTaskDelayUntil() returns
   pdFALSE without waiting. The cycles that follow run back to back until the task catches up with
   its original schedule. */

#define CYCLES 9

static void periodic_task(void *arg)
{
    static const char *const start_label[CYCLES] = {
        "cycle 0 starts at, microseconds", "cycle 1 starts at, microseconds",
        "cycle 2 starts at, microseconds", "cycle 3 starts at, microseconds",
        "cycle 4 starts at, microseconds", "cycle 5 starts at, microseconds",
        "cycle 6 starts at, microseconds", "cycle 7 starts at, microseconds",
        "cycle 8 starts at, microseconds"};
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        rt_say_u32(start_label[cycle], rt_now_us());
        rt_busy_us(cycle == 3 ? 24000u : 3000u);
        if (xTaskDelayUntil(&last, pdMS_TO_TICKS(10)) == pdFALSE) {
            rt_say("  xTaskDelayUntil returned pdFALSE: the wake time had passed");
        }
    }
    rt_finish(0);
}

int main(void)
{
    (void)xTaskCreate(periodic_task, "periodic", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
