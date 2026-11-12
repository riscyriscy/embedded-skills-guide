#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "rt.h"

/* Queue calls that the earlier programs did not use. A mailbox is a queue of length 1 that the
   producer overwrites. The other calls are on a queue of 4 items. The check task shows that
   xQueueReset releases a task that is blocked in a send. */

static QueueHandle_t mailbox, fifo, small;

static void show(const char *label, QueueHandle_t q)
{
    rt_say_u32(label, (uint32_t)uxQueueMessagesWaiting(q));
    rt_say_u32("  free slots", (uint32_t)uxQueueSpacesAvailable(q));
}

static uint32_t take(QueueHandle_t q)
{
    uint32_t v = 0;
    return xQueueReceive(q, &v, 0) == pdTRUE ? v : 0xFFFFFFFFu;
}

static void blocked_sender(void *arg)
{
    (void)arg;
    uint32_t v = 77;
    rt_say("blocked sender: calls xQueueSend on a full queue, waits");
    BaseType_t r = xQueueSend(small, &v, pdMS_TO_TICKS(200));
    rt_say_u32("blocked sender: xQueueSend returned", (uint32_t)r);
    vTaskDelete(NULL);
}

static void main_task(void *arg)
{
    (void)arg;
    uint32_t v;

    /* The mailbox: the latest value wins. */
    show("mailbox, items before", mailbox);
    for (v = 10; v <= 30; v += 10) {
        (void)xQueueOverwrite(mailbox, &v);
    }
    show("mailbox after three xQueueOverwrite", mailbox);
    v = 0;
    (void)xQueuePeek(mailbox, &v, 0);
    rt_say_u32("xQueuePeek returned the value", v);
    show("mailbox after the peek", mailbox);
    rt_say_u32("xQueueReceive returned the value", take(mailbox));
    show("mailbox after the receive", mailbox);
    rt_say_u32("xQueuePeek on the empty mailbox returned", (uint32_t)xQueuePeek(mailbox, &v, 0));
    v = 1;
    rt_say_u32("xQueueSend into the empty mailbox returned", (uint32_t)xQueueSend(mailbox, &v, 0));
    v = 2;
    rt_say_u32("xQueueSend into the full mailbox returned", (uint32_t)xQueueSend(mailbox, &v, 0));

    /* A queue of 4: send to the front, fill, reset. */
    for (v = 1; v <= 3; v++) {
        (void)xQueueSendToBack(fifo, &v, 0);
    }
    v = 99;
    (void)xQueueSendToFront(fifo, &v, 0);
    show("queue after 1, 2, 3 to the back and 99 to the front", fifo);
    v = 5;
    rt_say_u32("xQueueSend into the full queue returned", (uint32_t)xQueueSend(fifo, &v, 0));
    rt_say_u32("first item received", take(fifo));
    rt_say_u32("second item received", take(fifo));
    rt_say_u32("third item received", take(fifo));
    rt_say_u32("fourth item received", take(fifo));
    rt_say_u32("fifth receive on the empty queue, 4294967295 means none", take(fifo));
    for (v = 5; v <= 6; v++) {
        (void)xQueueSendToBack(fifo, &v, 0);
    }
    (void)xQueueReset(fifo);
    show("queue after two sends and xQueueReset", fifo);

    /* A reset releases a task that is blocked in a send. */
    v = 50;
    (void)xQueueSendToBack(small, &v, 0);
    (void)xTaskCreate(blocked_sender, "sender", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
    show("small queue before the reset", small);
    (void)xQueueReset(small);
    vTaskDelay(pdMS_TO_TICKS(10));
    show("small queue after the reset and the release of the sender", small);
    rt_say_u32("item in it", take(small));
    rt_finish(0);
}

int main(void)
{
    mailbox = xQueueCreate(1, sizeof(uint32_t));
    fifo = xQueueCreate(4, sizeof(uint32_t));
    small = xQueueCreate(1, sizeof(uint32_t));
    (void)xTaskCreate(main_task, "main", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
