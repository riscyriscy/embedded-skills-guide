#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "rt.h"

/* Priority inversion. A low priority task holds a lock, a high priority task wants it, and a task
   of medium priority that does not use the lock keeps the processor busy. Build with USE_MUTEX=1
   for a mutex, which lends its priority to the holder, and with USE_MUTEX=0 for a binary
   semaphore, which does not. */

#ifndef USE_MUTEX
#define USE_MUTEX 0
#endif

static SemaphoreHandle_t lock;

static void low_task(void *arg)
{
    (void)arg;
    (void)xSemaphoreTake(lock, portMAX_DELAY);
    rt_say("low takes the lock, needs 20 ms of work");
    rt_busy_us(20000);
    rt_say("low gives the lock back");
    (void)xSemaphoreGive(lock);
    vTaskDelay(portMAX_DELAY);
}

static void medium_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10));
    rt_say("medium starts a job of 100 ms");
    rt_busy_us(100000);
    rt_say("medium is done");
    vTaskDelay(portMAX_DELAY);
}

static void high_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(5));
    uint32_t asked = rt_now_us();
    rt_say("high asks for the lock");
    (void)xSemaphoreTake(lock, portMAX_DELAY);
    uint32_t waited = rt_now_us() - asked;
    rt_say_u32("high has the lock, microseconds waited", waited);
    (void)xSemaphoreGive(lock);
    vTaskDelay(pdMS_TO_TICKS(150));
    rt_finish(0);
}

int main(void)
{
#if USE_MUTEX
    lock = xSemaphoreCreateMutex();
#else
    lock = xSemaphoreCreateBinary();
    (void)xSemaphoreGive(lock);
#endif
    (void)xTaskCreate(low_task, "low", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTaskCreate(medium_task, "medium", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    (void)xTaskCreate(high_task, "high", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
