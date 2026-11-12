#include <stdint.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "rt.h"

/* Static allocation. The program creates a task, a queue, an event group, a binary semaphore and a
   timer from memory that it supplies, and shows that the heap does not change. It then creates the
   same five objects from the heap and shows what each takes. The kernel build is the one of
   config_x, and the memory of the idle task and the timer task comes from src/static_mem.c. */

static StaticTask_t worker_tcb;
static StackType_t worker_stack[configMINIMAL_STACK_SIZE];
static StaticQueue_t queue_control;
static uint8_t queue_storage[4 * sizeof(uint32_t)];
static StaticEventGroup_t group_control;
static StaticSemaphore_t semaphore_control;
static StaticTimer_t timer_control;

static QueueHandle_t queue;
static EventGroupHandle_t group;
static SemaphoreHandle_t semaphore;
static TimerHandle_t timer;
static size_t heap_at_start;

static void timer_callback(TimerHandle_t t)
{
    (void)t;
}

static void idle_like_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void say_cost(const char *label, size_t before)
{
    rt_say_u32(label, (uint32_t)(before - xPortGetFreeHeapSize()));
}

static void worker_task(void *arg)
{
    (void)arg;
    uint32_t value = 42, got = 0;

    rt_say_u32("bytes of StaticTask_t", sizeof(StaticTask_t));
    rt_say_u32("bytes of StaticQueue_t", sizeof(StaticQueue_t));
    rt_say_u32("bytes of StaticEventGroup_t", sizeof(StaticEventGroup_t));
    rt_say_u32("bytes of StaticSemaphore_t", sizeof(StaticSemaphore_t));
    rt_say_u32("bytes of StaticTimer_t", sizeof(StaticTimer_t));
    say_cost("heap bytes taken by five static objects, the idle task and the timer task",
             heap_at_start);

    /* The static objects work like the others. */
    (void)xQueueSend(queue, &value, 0);
    (void)xQueueReceive(queue, &got, 0);
    rt_say_u32("value through the static queue", got);
    (void)xEventGroupSetBits(group, 5u);
    rt_say_u32("bits from the static event group",
               (uint32_t)xEventGroupWaitBits(group, 5u, pdTRUE, pdTRUE, 0));
    (void)xSemaphoreGive(semaphore);
    rt_say_u32("xSemaphoreTake on the static semaphore returned",
               (uint32_t)xSemaphoreTake(semaphore, 0));
    (void)xTimerStart(timer, 0);
    rt_say_u32("static timer active", (uint32_t)xTimerIsTimerActive(timer));
    (void)xTimerStop(timer, 0);

    /* The same objects from the heap. */
    size_t before = xPortGetFreeHeapSize();
    (void)xTaskCreate(idle_like_task, "dyn", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    say_cost("heap bytes taken by xTaskCreate with a stack of 128 words", before);
    before = xPortGetFreeHeapSize();
    (void)xQueueCreate(4, sizeof(uint32_t));
    say_cost("heap bytes taken by xQueueCreate of 4 items of 4 bytes", before);
    before = xPortGetFreeHeapSize();
    (void)xEventGroupCreate();
    say_cost("heap bytes taken by xEventGroupCreate", before);
    before = xPortGetFreeHeapSize();
    (void)xSemaphoreCreateBinary();
    say_cost("heap bytes taken by xSemaphoreCreateBinary", before);
    before = xPortGetFreeHeapSize();
    (void)xTimerCreate("dyn", pdMS_TO_TICKS(10), pdFALSE, NULL, timer_callback);
    say_cost("heap bytes taken by xTimerCreate", before);
    rt_finish(0);
}

int main(void)
{
    vPortFree(pvPortMalloc(1)); /* heap_4 sets up its free list at the first allocation */
    heap_at_start = xPortGetFreeHeapSize();
    queue = xQueueCreateStatic(4, sizeof(uint32_t), queue_storage, &queue_control);
    group = xEventGroupCreateStatic(&group_control);
    semaphore = xSemaphoreCreateBinaryStatic(&semaphore_control);
    timer = xTimerCreateStatic("static", pdMS_TO_TICKS(10), pdFALSE, NULL, timer_callback,
                               &timer_control);
    (void)xTaskCreateStatic(worker_task, "worker", configMINIMAL_STACK_SIZE, NULL, 2, worker_stack,
                            &worker_tcb);
    vTaskStartScheduler();
    for (;;) {
    }
}
