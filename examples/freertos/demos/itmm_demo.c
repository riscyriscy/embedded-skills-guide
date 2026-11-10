#include "FreeRTOS.h"
#include "task.h"

#include "itmm.h"
#include "rt.h"

/* The calls of ITMM, one at a time. The controller drives the steps and the worker answers. The
   worker has the lower priority, so messages that the controller sends in a row are in the queue
   when the worker runs. */

enum { T_CONTROL, T_WORKER, TASKS };
enum { MSG_NOTE = 1, MSG_JOB = 2 };

#define EVT_CONFIG (1u << 0)
#define EVT_STOP   (1u << 1)
#define EVT_TICK   (1u << 2)

static uint32_t heap_for_itmm;

static void say_status(const char *label, itmm_status_t status)
{
    rt_say_str(label, itmm_status_text(status));
}

static void control_task(void *arg)
{
    (void)arg;
    itmm_msg_t m = {.sender = T_CONTROL, .type = MSG_NOTE};

    rt_say_u32("bytes of heap that itmm_init took for 2 tasks with 4 slots each", heap_for_itmm);
    rt_say_u32("bytes in one message", sizeof(itmm_msg_t));

    /* 1. Three messages in a row: the worker wakes once and takes all three. */
    vTaskDelay(pdMS_TO_TICKS(10));
    for (uint32_t i = 1; i <= 3; i++) {
        m.value = i;
        (void)itmm_send(T_WORKER, &m, 0);
    }
    rt_say("controller sent three notes");

    /* 2. A message that the controller waits for: the worker needs 20 ms, the wait is 100 ms. */
    vTaskDelay(pdMS_TO_TICKS(40));
    m.type = MSG_JOB;
    m.value = 20;
    uint32_t start = rt_now_us();
    say_status("blocking send, 20 ms job, 100 ms wait",
               itmm_send_wait(T_WORKER, &m, 0, pdMS_TO_TICKS(100)));
    rt_say_u32("milliseconds that the send took", (rt_now_us() - start) / 1000u);

    /* 3. The same with a job of 60 ms and a wait of 30 ms: the controller gives up first. */
    m.value = 60;
    start = rt_now_us();
    say_status("blocking send, 60 ms job, 30 ms wait",
               itmm_send_wait(T_WORKER, &m, 0, pdMS_TO_TICKS(30)));
    rt_say_u32("milliseconds that the send took", (rt_now_us() - start) / 1000u);

    /* 4. Two flags that the worker waits for together. */
    vTaskDelay(pdMS_TO_TICKS(100));
    (void)itmm_event_set(T_WORKER, EVT_CONFIG);
    rt_say("controller set CONFIG");
    vTaskDelay(pdMS_TO_TICKS(30));
    (void)itmm_event_set(T_WORKER, EVT_STOP);
    rt_say("controller set STOP");

    /* 5. A flag that is set and cleared before the worker looks at it. */
    vTaskDelay(pdMS_TO_TICKS(50));
    (void)itmm_event_set(T_WORKER, EVT_TICK);
    rt_say_u32("flags of the worker after set TICK", itmm_event_get(T_WORKER));
    (void)itmm_event_clear(T_WORKER, EVT_TICK);
    rt_say_u32("flags of the worker after clear TICK", itmm_event_get(T_WORKER));

    /* 6. A message that arrives while the worker waits for a flag that is not the message flag. */
    vTaskDelay(pdMS_TO_TICKS(40));
    m.type = MSG_NOTE;
    m.value = 99;
    (void)itmm_send(T_WORKER, &m, 0);
    rt_say("controller sent a note");

    /* 7. A flag number that the application may not use. */
    vTaskDelay(pdMS_TO_TICKS(100));
    say_status("set the message flag from the application",
               itmm_event_set(T_WORKER, ITMM_EVT_MESSAGE));
    vTaskDelay(pdMS_TO_TICKS(20));
    rt_finish(0);
}

static void worker_task(void *arg)
{
    (void)arg;
    itmm_msg_t m;
    itmm_events_t ev;

    /* 1 */
    ev = itmm_wait(T_WORKER, ITMM_EVT_MESSAGE, 0, portMAX_DELAY);
    rt_say_u32("worker woke, flags returned", ev);
    uint32_t taken = 0;
    while (itmm_receive(T_WORKER, &m) == ITMM_OK) {
        taken++;
        rt_say_u32("worker took note", m.value);
    }
    rt_say_u32("messages taken in this wake", taken);
    rt_say_u32("flags of the worker after the last receive", itmm_event_get(T_WORKER));

    /* 2 and 3: the second job is queued while the worker is still in the loop. */
    (void)itmm_wait(T_WORKER, ITMM_EVT_MESSAGE, 0, portMAX_DELAY);
    while (itmm_receive(T_WORKER, &m) == ITMM_OK) {
        rt_say_u32("worker starts job of milliseconds", m.value);
        vTaskDelay(pdMS_TO_TICKS(m.value));
        say_status("worker done", itmm_done(&m));
    }

    /* 4 */
    ev = itmm_wait(T_WORKER, EVT_CONFIG | EVT_STOP, ITMM_WAIT_ALL, pdMS_TO_TICKS(200));
    rt_say_u32("worker waited for CONFIG and STOP, flags returned", ev);
    rt_say_u32("flags of the worker after the wait", itmm_event_get(T_WORKER));

    /* 5 */
    vTaskDelay(pdMS_TO_TICKS(60));
    ev = itmm_wait(T_WORKER, EVT_TICK, 0, pdMS_TO_TICKS(20));
    rt_say_u32("worker waited for TICK, flags returned", ev);

    /* 6 */
    ev = itmm_wait(T_WORKER, EVT_CONFIG, 0, pdMS_TO_TICKS(60));
    rt_say_u32("worker waited for CONFIG only, flags returned", ev);
    say_status("receive after that wait", itmm_receive(T_WORKER, &m));
    rt_say_u32("value of the note", m.value);
    say_status("receive again", itmm_receive(T_WORKER, &m));
    rt_say_u32("flags of the worker after the second receive", itmm_event_get(T_WORKER));

    vTaskDelete(NULL);
}

int main(void)
{
    static const itmm_config_t cfg = {.task_count = TASKS, .queue_depth = {4, 4}};
    vPortFree(pvPortMalloc(1)); /* heap_4 sets up its free list at the first allocation */
    size_t heap_before = xPortGetFreeHeapSize();
    if (itmm_init(&cfg) != ITMM_OK) {
        rt_say("itmm_init failed");
        rt_finish(1);
    }
    heap_for_itmm = (uint32_t)(heap_before - xPortGetFreeHeapSize());
    (void)xTaskCreate(control_task, "control", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    (void)xTaskCreate(worker_task, "worker", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {
    }
}
