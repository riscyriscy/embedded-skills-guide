#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* A stack of 96 words (384 bytes) that is too small for a recursive function, and the check that
   configCHECK_FOR_STACK_OVERFLOW = 2 makes at every task switch. */

static uint32_t depth_reached;

static uint32_t fill(uint32_t depth)
{
    volatile uint8_t buffer[120];
    for (unsigned i = 0; i < sizeof buffer; i++) {
        buffer[i] = (uint8_t)(depth + i);
    }
    depth_reached = depth;
    if (depth < 2u) {
        return fill(depth + 1u) + buffer[depth];
    }
    return buffer[0];
}

static void healthy_task(void *arg)
{
    (void)arg;
    volatile uint8_t local[64] = {0};
    (void)local;
    rt_say_u32("healthy: stack words that were never used",
               (uint32_t)uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(5));
    rt_say("healthy: the stack of 128 words is large enough");
    vTaskDelay(portMAX_DELAY);
}

static void hungry_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(20));
    rt_say("hungry: calls a function that recurses 3 times with 120 bytes each");
    (void)fill(0);
    rt_say_u32("hungry: returned, deepest level was", depth_reached);
    rt_say_u32("hungry: stack words that were never used",
               (uint32_t)uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(pdMS_TO_TICKS(10));
    rt_say("hungry: still running");
    vTaskDelay(portMAX_DELAY);
}

int main(void)
{
    (void)xTaskCreate(healthy_task, "healthy", 128, NULL, 1, NULL);
    (void)xTaskCreate(hungry_task, "hungry", 96, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
