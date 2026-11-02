#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "timers.h"

#include "rt.h"

/* A software timer that wakes a task with a notification, and an event group that releases a task
   when two other tasks have both reported. */

#define BIT_SENSOR (1u << 0)
#define BIT_LINK   (1u << 1)

static EventGroupHandle_t ready;
static TaskHandle_t worker;
static TimerHandle_t timer;

static void timer_callback(TimerHandle_t t)
{
    (void)t;
    (void)xTaskNotifyGive(worker);
}

static void worker_task(void *arg)
{
    (void)arg;
    for (uint32_t i = 1; i <= 3; i++) {
        uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rt_say_u32("worker notified by the timer, notifications taken", count);
    }
    (void)xTimerStop(timer, portMAX_DELAY);
    rt_say("timer stopped");
    vTaskDelete(NULL);
}

static void sensor_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(130));
    rt_say("sensor is ready");
    (void)xEventGroupSetBits(ready, BIT_SENSOR);
    vTaskDelay(portMAX_DELAY);
}

static void link_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(210));
    rt_say("link is up");
    (void)xEventGroupSetBits(ready, BIT_LINK);
    vTaskDelay(portMAX_DELAY);
}

static void startup_task(void *arg)
{
    (void)arg;
    EventBits_t bits =
        xEventGroupWaitBits(ready, BIT_SENSOR | BIT_LINK, pdFALSE, pdTRUE, pdMS_TO_TICKS(100));
    rt_say_u32("first wait for both, bits set when it returned", bits);
    bits = xEventGroupWaitBits(ready, BIT_SENSOR | BIT_LINK, pdTRUE, pdTRUE, pdMS_TO_TICKS(500));
    rt_say_u32("second wait for both, bits set when it returned", bits);
    rt_say_u32("bits left after the wait cleared them", xEventGroupGetBits(ready));
    vTaskDelay(pdMS_TO_TICKS(150));
    rt_finish(0);
}

int main(void)
{
    ready = xEventGroupCreate();
    timer = xTimerCreate("tick", pdMS_TO_TICKS(100), pdTRUE, NULL, timer_callback);
    (void)xTaskCreate(worker_task, "worker", configMINIMAL_STACK_SIZE, NULL, 2, &worker);
    (void)xTaskCreate(sensor_task, "sensor", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTaskCreate(link_task, "link", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTaskCreate(startup_task, "startup", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    (void)xTimerStart(timer, 0);
    vTaskStartScheduler();
    for (;;) {
    }
}
