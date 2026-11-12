#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "rt.h"

/* A task notification used as a group of 32 flags. The receiver waits with xTaskNotifyWait(). The
   controller sets flags with xTaskNotify() and an interrupt handler sets one with
   xTaskNotifyFromISR(). Flag values are bit masks and the output shows them as numbers. The program
   links the kernel of config_x, which has two notification indexes. */

#define ALL_BITS 0xFFFFFFFFu

static TaskHandle_t receiver;

void Irq0_Handler(void)
{
    BaseType_t woken = pdFALSE;
    (void)xTaskNotifyFromISR(receiver, 16u, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

static void wait_and_say(const char *label, UBaseType_t index, uint32_t clear_on_entry,
                         uint32_t clear_on_exit, TickType_t timeout)
{
    uint32_t value = 0;
    if (xTaskNotifyWaitIndexed(index, clear_on_entry, clear_on_exit, &value, timeout) == pdTRUE) {
        rt_say_u32(label, value);
    } else {
        rt_say_str(label, "timed out");
    }
}

static void controller_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(10)); /* 1. two flags before the receiver looks */
    (void)xTaskNotify(receiver, 1u, eSetBits);
    (void)xTaskNotify(receiver, 4u, eSetBits);
    rt_say("controller set flag values 1 and 4");

    vTaskDelay(pdMS_TO_TICKS(20)); /* 2. a flag from an interrupt handler */
    rt_irq_pend(0);

    vTaskDelay(pdMS_TO_TICKS(20)); /* 3. a flag that stays in the value after the wait */
    (void)xTaskNotify(receiver, 128u, eSetBits);
    rt_say("controller set flag value 128");

    vTaskDelay(pdMS_TO_TICKS(20));
    (void)xTaskNotify(receiver, 1u, eSetBits);
    rt_say("controller set flag value 1");

    vTaskDelay(pdMS_TO_TICKS(20)); /* 4. a new flag for a receiver that cleared the old ones */
    (void)xTaskNotify(receiver, 2u, eSetBits);
    rt_say("controller set flag value 2");

    vTaskDelay(pdMS_TO_TICKS(30)); /* 5. a flag on index 1 */
    (void)xTaskNotifyIndexed(receiver, 1, 2u, eSetBits);
    rt_say("controller set flag value 2 on index 1");

    vTaskDelay(pdMS_TO_TICKS(20)); /* 6. a value that must not be overwritten */
    BaseType_t first = xTaskNotify(receiver, 7u, eSetValueWithoutOverwrite);
    BaseType_t second = xTaskNotify(receiver, 8u, eSetValueWithoutOverwrite);
    rt_say_u32("first xTaskNotify with eSetValueWithoutOverwrite returned", (uint32_t)first);
    rt_say_u32("second xTaskNotify with eSetValueWithoutOverwrite returned", (uint32_t)second);
    vTaskDelay(pdMS_TO_TICKS(30));
    rt_finish(0);
}

static void receiver_task(void *arg)
{
    (void)arg;
    TickType_t t100 = pdMS_TO_TICKS(100);
    TickType_t t20 = pdMS_TO_TICKS(20);

    wait_and_say("wait, clear all on exit, value", 0, 0, ALL_BITS, t100); /* 1 */
    wait_and_say("wait, clear all on exit, value", 0, 0, ALL_BITS, t100); /* 2 */

    vTaskDelay(pdMS_TO_TICKS(30)); /* 3: now at 60 ms */
    wait_and_say("wait, clear nothing on exit, value", 0, 0, 0, t20);
    vTaskDelay(pdMS_TO_TICKS(20)); /* now at 80 ms: the flag value 1 came at 70 ms */
    wait_and_say("wait, clear nothing on exit, value", 0, 0, 0, t20);

    /* 4: bits 128 and 1 are in the value. Clearing them on entry makes the wait see flag value 2
       only. The notification value does not hold a pending flag now, so the clear on entry works.
     */
    wait_and_say("wait, clear 129 on entry, value", 0, 129u, ALL_BITS, t100);

    wait_and_say("wait on index 0 for 20 ms, result", 0, 0, ALL_BITS, t20); /* 5 */
    wait_and_say("wait on index 1, value", 1, 0, ALL_BITS, t20);

    vTaskDelay(pdMS_TO_TICKS(30)); /* 6 */
    wait_and_say("wait after the two sends, value", 0, 0, ALL_BITS, t20);
    vTaskDelete(NULL);
}

int main(void)
{
    rt_irq_setup(0, 192);
    (void)xTaskCreate(receiver_task, "receiver", configMINIMAL_STACK_SIZE, NULL, 2, &receiver);
    (void)xTaskCreate(controller_task, "controller", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
