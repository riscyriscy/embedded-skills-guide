#include "FreeRTOS.h"
#include "task.h"

#include "itmm.h"
#include "rt.h"

/* An interrupt handler that talks to a task through ITMM. The device interrupt sends one message
   for each sample and sets the flag OVERRUN when the queue of the task is full. The sensor task
   stands for the device: it pends interrupt 0 every 5 ms. The consumer needs 12 ms for each sample,
   so the queue of 3 slots fills. The consumer waits for the message flag, for OVERRUN and for a
   time out. */

enum { T_CONSUMER, TASKS };
enum { MSG_SAMPLE = 1 };

#define EVT_OVERRUN (1u << 0)

static volatile uint32_t produced, dropped, overruns_posted;
static uint32_t stamp_us[16]; /* the time of each interrupt, by sample number */

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    uint32_t seq = ++produced;
    stamp_us[seq % 16u] = rt_now_us();
    itmm_msg_t m = {.type = MSG_SAMPLE, .value = seq};
    if (itmm_send_from_isr(T_CONSUMER, &m, &woken) == ITMM_ERR_FULL) {
        dropped++;
        if (itmm_event_set_from_isr(T_CONSUMER, EVT_OVERRUN, &woken) == ITMM_OK) {
            overruns_posted++;
        }
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
    vTaskDelete(NULL);
}

static void consumer_task(void *arg)
{
    (void)arg;
    uint32_t received = 0, overruns_seen = 0;
    for (;;) {
        itmm_events_t ev =
            itmm_wait(T_CONSUMER, ITMM_EVT_MESSAGE | EVT_OVERRUN, 0, pdMS_TO_TICKS(200));
        if (ev == 0u) {
            break;
        }
        if ((ev & EVT_OVERRUN) != 0u) {
            overruns_seen++;
            rt_say("consumer sees OVERRUN");
        }
        itmm_msg_t m;
        while (itmm_receive(T_CONSUMER, &m) == ITMM_OK) {
            received++;
            rt_say_u32("consumer has sample", m.value);
            if (received == 1u) {
                rt_say_u32("microseconds from the interrupt to the task",
                           rt_now_us() - stamp_us[m.value % 16u]);
            }
            rt_busy_us(12000);
        }
    }
    rt_say_u32("samples produced", produced);
    rt_say_u32("samples received", received);
    rt_say_u32("samples dropped by the interrupt", dropped);
    rt_say_u32("OVERRUN flags posted by the interrupt", overruns_posted);
    rt_say_u32("wakes with OVERRUN set", overruns_seen);
    rt_finish(0);
}

int main(void)
{
    static const itmm_config_t cfg = {.task_count = TASKS, .queue_depth = {3}};
    if (itmm_init(&cfg) != ITMM_OK) {
        rt_say("itmm_init failed");
        rt_finish(1);
    }
    /* Priority 192 is less urgent than configMAX_SYSCALL_INTERRUPT_PRIORITY (160): the interrupt
     * may call FromISR functions. */
    rt_irq_setup(0, 192);
    (void)xTaskCreate(sensor_task, "sensor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    (void)xTaskCreate(consumer_task, "consumer", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
