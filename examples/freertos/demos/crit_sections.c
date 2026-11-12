#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "rt.h"
#include "rt_cyc.h"

/* What a critical section and a suspended scheduler hold back. Interrupt 0 has priority 192, so the
   kernel may mask it, and it gives a semaphore to a waiting task. Interrupt 1 has priority 32, more
   urgent than the kernel's limit of 160, and calls nothing of the kernel. In three cases the driver
   task pends both interrupts and then computes for 10000 cycles, 400 microseconds. The times that
   the output shows count from the moment of the pend. */

static SemaphoreHandle_t sem;
static volatile uint32_t t0, at_irq0, at_irq1, at_task;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    at_irq0 = rt_cyc_since(t0);
    (void)xSemaphoreGiveFromISR(sem, &woken);
    portYIELD_FROM_ISR(woken);
}

void Irq1_Handler(void)
{
    at_irq1 = rt_cyc_since(t0);
}

static void waiter_task(void *arg)
{
    (void)arg;
    for (;;) {
        (void)xSemaphoreTake(sem, portMAX_DELAY);
        at_task = rt_cyc_since(t0);
    }
}

static void report(const char *what)
{
    rt_say(what);
    rt_say_u32("  interrupt 0 ran after, in microseconds", at_irq0 / 25u);
    rt_say_u32("  interrupt 1 ran after, in microseconds", at_irq1 / 25u);
    rt_say_u32("  the waiting task ran after, in microseconds", at_task / 25u);
}

static void driver_task(void *arg)
{
    (void)arg;
    for (int mode = 0; mode < 3; mode++) {
        vTaskDelay(pdMS_TO_TICKS(5)); /* align to a tick, so that the case does not cross one */
        t0 = rt_cyc_now();
        if (mode == 1) {
            taskENTER_CRITICAL();
        } else if (mode == 2) {
            vTaskSuspendAll();
        }
        rt_irq_pend(1);
        rt_irq_pend(0);
        rt_cyc_spin(10000);
        if (mode == 1) {
            taskEXIT_CRITICAL();
        } else if (mode == 2) {
            (void)xTaskResumeAll();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        report(mode == 0   ? "no protection"
               : mode == 1 ? "inside taskENTER_CRITICAL, 400 us"
                           : "inside vTaskSuspendAll, 400 us");
    }
    rt_finish(0);
}

int main(void)
{
    sem = xSemaphoreCreateBinary();
    rt_irq_setup(0, 192);
    rt_irq_setup(1, 32);
    (void)xTaskCreate(waiter_task, "waiter", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    (void)xTaskCreate(driver_task, "driver", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
