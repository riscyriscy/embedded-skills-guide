#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "rt.h"
#include "rt_cyc.h"

/* Four ways to hand the work of an interrupt to a task, timed from the moment that the driver
   pends the interrupt to the moment that the first instruction of the work runs. The handler has
   priority 192 and the task that the handler wakes has priority 3. The timer task has priority 4,
   so the last way runs the work in a task that is more urgent than the task of the other ways. Each
   way runs 8 times. */

#define RUNS 8u

enum { BY_SEMAPHORE, BY_NOTIFY, BY_QUEUE, BY_TIMER, WAYS };

static SemaphoreHandle_t sem;
static QueueHandle_t queue;
static TaskHandle_t worker;
static TaskHandle_t driver;
static volatile uint32_t way, t0, done_at;

static void deferred_function(void *arg, uint32_t value)
{
    (void)arg;
    (void)value;
    done_at = rt_cyc_since(t0);
    xTaskNotifyGive(driver);
}

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    switch (way) {
    case BY_SEMAPHORE:
        (void)xSemaphoreGiveFromISR(sem, &woken);
        break;
    case BY_NOTIFY:
        vTaskNotifyGiveFromISR(worker, &woken);
        break;
    case BY_QUEUE: {
        uint32_t item = 1;
        (void)xQueueSendFromISR(queue, &item, &woken);
        break;
    }
    default:
        (void)xTimerPendFunctionCallFromISR(deferred_function, NULL, 0, &woken);
        break;
    }
    portYIELD_FROM_ISR(woken);
}

static void sem_task(void *arg)
{
    (void)arg;
    for (;;) {
        (void)xSemaphoreTake(sem, portMAX_DELAY);
        done_at = rt_cyc_since(t0);
        xTaskNotifyGive(driver);
    }
}

static void notify_task(void *arg)
{
    (void)arg;
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        done_at = rt_cyc_since(t0);
        xTaskNotifyGive(driver);
    }
}

static void queue_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t item;
        (void)xQueueReceive(queue, &item, portMAX_DELAY);
        done_at = rt_cyc_since(t0);
        xTaskNotifyGive(driver);
    }
}

static void driver_task(void *arg)
{
    static const char *const name[WAYS] = {
        "xSemaphoreGiveFromISR and xSemaphoreTake", "vTaskNotifyGiveFromISR and ulTaskNotifyTake",
        "xQueueSendFromISR and xQueueReceive", "xTimerPendFunctionCallFromISR"};
    (void)arg;
    for (uint32_t w = 0; w < WAYS; w++) {
        uint32_t first = 0, differing = 0;
        way = w;
        for (uint32_t i = 0; i < RUNS; i++) {
            vTaskDelay(pdMS_TO_TICKS(3));
            t0 = rt_cyc_now();
            rt_irq_pend(0);
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if (i == 0u) {
                first = done_at;
            } else if (done_at != first) {
                differing++;
            }
        }
        rt_say(name[w]);
        rt_say_u32("  cycles from the pend to the work", first);
        rt_say_u32("  runs of 8 that took another time", differing);
    }
    rt_finish(0);
}

int main(void)
{
    sem = xSemaphoreCreateBinary();
    queue = xQueueCreate(2, sizeof(uint32_t));
    rt_irq_setup(0, 192);
    (void)xTaskCreate(sem_task, "sem", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    (void)xTaskCreate(notify_task, "notify", configMINIMAL_STACK_SIZE, NULL, 3, &worker);
    (void)xTaskCreate(queue_task, "queue", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    (void)xTaskCreate(driver_task, "driver", configMINIMAL_STACK_SIZE, NULL, 2, &driver);
    vTaskStartScheduler();
    for (;;) {
    }
}
