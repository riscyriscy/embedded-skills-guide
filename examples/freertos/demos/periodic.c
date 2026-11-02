#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* Three periodic tasks with fixed priorities, released together at tick 10 (the critical instant),
   and the worst response time of each over one second: from the release to the end of the job. */

typedef struct {
    const char *name;
    uint32_t period_ms;
    uint32_t work_us;
    UBaseType_t priority;
    uint32_t worst_us;
    uint32_t jobs;
} spec_t;

static spec_t specs[3] = {
    {"t1", 10, 2000, 3, 0, 0},
    {"t2", 20, 5000, 2, 0, 0},
    {"t3", 50, 12000, 1, 0, 0},
};

#define START_TICK 10u
#define END_TICK   1010u

static void periodic_task(void *arg)
{
    spec_t *s = arg;
    TickType_t release = START_TICK;
    vTaskDelay(START_TICK - xTaskGetTickCount());
    while (release < END_TICK) {
        rt_busy_us(s->work_us);
        uint32_t response = rt_now_us() - (uint32_t)release * 1000u;
        if (response > s->worst_us) {
            s->worst_us = response;
        }
        s->jobs++;
        vTaskDelayUntil(&release, pdMS_TO_TICKS(s->period_ms));
    }
    vTaskDelay(portMAX_DELAY);
}

static void report_task(void *arg)
{
    (void)arg;
    vTaskDelay(END_TICK + 60u);
    for (int i = 0; i < 3; i++) {
        rt_say_u32(specs[i].name, specs[i].worst_us);
    }
    rt_finish(0);
}

int main(void)
{
    for (int i = 0; i < 3; i++) {
        (void)xTaskCreate(periodic_task, specs[i].name, configMINIMAL_STACK_SIZE, &specs[i],
                          specs[i].priority, NULL);
    }
    (void)xTaskCreate(report_task, "report", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
