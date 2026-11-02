#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "rt.h"

/* A device interrupt hands samples to a task through a queue. The sensor task stands for the
   device: it pends interrupt 0 every 5 ms. The consumer needs 12 ms per sample, so the queue fills.
 */

typedef struct {
    uint32_t seq;
    uint32_t t_us; /* the time of the interrupt */
} sample_t;

static QueueHandle_t samples;
static volatile uint32_t produced, dropped;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    sample_t s = {++produced, rt_now_us()};
    if (xQueueSendFromISR(samples, &s, &woken) != pdTRUE) {
        dropped++;
    }
    portYIELD_FROM_ISR(woken);
}

static void sensor_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(2));
    for (int i = 0; i < 8; i++) {
        rt_irq_pend(0);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(NULL); /* a task function must not return */
}

static void consumer_task(void *arg)
{
    (void)arg;
    uint32_t received = 0;
    sample_t s;
    while (xQueueReceive(samples, &s, pdMS_TO_TICKS(200)) == pdTRUE) {
        received++;
        rt_say_u32("consumer has sample", s.seq);
        if (received == 1u) {
            rt_say_u32("microseconds from the interrupt to the task", rt_now_us() - s.t_us);
        }
        rt_busy_us(12000);
    }
    rt_say_u32("samples produced", produced);
    rt_say_u32("samples received", received);
    rt_say_u32("samples dropped by the interrupt", dropped);
    rt_say_u32("queue slots", 3);
    rt_finish(0);
}

int main(void)
{
    samples = xQueueCreate(3, sizeof(sample_t));
    /* Priority 192 is less urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY (160): the interrupt
     * may call FromISR functions. */
    rt_irq_setup(0, 192);
    (void)xTaskCreate(sensor_task, "sensor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    (void)xTaskCreate(consumer_task, "consumer", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
