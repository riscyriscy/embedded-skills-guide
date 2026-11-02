#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* Interrupt to task latency. Every 10 ms the tick hook pends interrupt 0. The handler reads the
   SysTick counter and notifies the task, and the task reads the counter again. Eight runs. The same
   job is in bare_latency.c without a kernel and in the Zephyr example latency. */

#define SYST_CVR (*(volatile uint32_t *)0xE000E018u)
#define RELOAD   25000u /* counts in one tick */

static volatile uint32_t stamp;
static TaskHandle_t handler;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    stamp = SYST_CVR;
    vTaskNotifyGiveFromISR(handler, &woken);
    portYIELD_FROM_ISR(woken);
}

void vApplicationTickHook(void)
{
    static uint32_t ticks;
    if (++ticks % 10u == 0u && ticks > 20u) {
        rt_irq_pend(0);
    }
}

static void handler_task(void *arg)
{
    (void)arg;
    uint32_t min = 0xFFFFFFFFu, max = 0, total = 0;
    for (int i = 0; i < 8; i++) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        uint32_t cycles = (stamp + RELOAD - SYST_CVR) % RELOAD;
        if (cycles < min) {
            min = cycles;
        }
        if (cycles > max) {
            max = cycles;
        }
        total += cycles;
    }
    rt_say_u32("interrupt to task, least, in cycles of 25 MHz", min);
    rt_say_u32("interrupt to task, greatest, in cycles of 25 MHz", max);
    rt_say_u32("interrupt to task, mean, in cycles of 25 MHz", total / 8u);
    rt_finish(0);
}

int main(void)
{
    rt_irq_setup(0, 192);
    (void)xTaskCreate(handler_task, "handler", configMINIMAL_STACK_SIZE, NULL, 3, &handler);
    vTaskStartScheduler();
    for (;;) {
    }
}
