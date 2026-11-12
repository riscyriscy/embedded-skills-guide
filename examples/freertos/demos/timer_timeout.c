#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "rt.h"

/* A software timer as a time out. The one shot timer link_timer stands for a link that must send a
   heartbeat at least every 50 ms. Each heartbeat restarts the count with xTimerReset(). When the
   count ends, the callback runs in the timer task. A second timer, tick_timer, runs every 10 ms and
   shows what a long callback does to the other timers. The program has four parts: heartbeats from
   a task, a change of the period, heartbeats from an interrupt handler, and a burst of commands
   that is larger than the timer queue. */

static TimerHandle_t link_timer, tick_timer;
static volatile uint32_t last_tick_us, timeouts;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    (void)xTimerResetFromISR(link_timer, &woken);
    portYIELD_FROM_ISR(woken);
}

static void tick_callback(TimerHandle_t timer)
{
    (void)timer;
    uint32_t now = rt_now_us();
    if (last_tick_us != 0u && now - last_tick_us > 12000u) {
        rt_say_u32("tick callback was late, microseconds since the previous one",
                   now - last_tick_us);
    }
    last_tick_us = now;
}

static void timeout_callback(TimerHandle_t timer)
{
    (void)timer;
    timeouts++;
    rt_say("time out: no heartbeat");
    if (timeouts == 1u) {
        rt_say_str("the callback runs in the task", pcTaskGetName(NULL));
        rt_say_u32("priority of that task", (uint32_t)uxTaskPriorityGet(NULL));
        rt_busy_us(15000); /* a callback that takes 15 ms: the tick timer waits */
    }
}

static void driver_task(void *arg)
{
    (void)arg;
    (void)xTimerStart(tick_timer, 0);

    /* 1. five heartbeats from a task, then silence */
    for (int i = 0; i < 5; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
        (void)xTimerReset(link_timer, 0);
        rt_say("heartbeat");
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 2. a shorter time out: the change also starts a timer that has ended */
    rt_say_u32("xTimerIsTimerActive after the time out", (uint32_t)xTimerIsTimerActive(link_timer));
    (void)xTimerChangePeriod(link_timer, pdMS_TO_TICKS(30), 0);
    rt_say("period changed to 30 ms, timer running again");
    vTaskDelay(pdMS_TO_TICKS(60));

    /* 3. three heartbeats from the interrupt handler, 10 ms apart */
    for (int i = 0; i < 3; i++) {
        rt_irq_pend(0);
        rt_say("heartbeat from the interrupt handler");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(60));

    /* 4. twelve commands while the scheduler is suspended: the timer queue holds 8 */
    uint32_t accepted = 0, refused = 0;
    vTaskSuspendAll();
    for (int i = 0; i < 12; i++) {
        if (xTimerReset(link_timer, 0) == pdPASS) {
            accepted++;
        } else {
            refused++;
        }
    }
    (void)xTaskResumeAll();
    rt_say_u32("commands accepted by the timer queue", accepted);
    rt_say_u32("commands refused", refused);
    vTaskDelay(pdMS_TO_TICKS(60));
    rt_say_u32("time outs in all", timeouts);
    rt_finish(0);
}

int main(void)
{
    link_timer = xTimerCreate("link", pdMS_TO_TICKS(50), pdFALSE, NULL, timeout_callback);
    tick_timer = xTimerCreate("tick", pdMS_TO_TICKS(10), pdTRUE, NULL, tick_callback);
    rt_irq_setup(0, 192);
    (void)xTaskCreate(driver_task, "driver", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
