#include "itmm.h"

#include <stddef.h>
#include <string.h>

#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef struct {
    QueueHandle_t queue;
    EventGroupHandle_t events;
    SemaphoreHandle_t reply;   /* given when a message that this task sent has been finished */
    volatile uint16_t pending; /* token of the message that this task waits for, or 0 */
    uint16_t last;             /* the last token handed out */
} slot_t;

static slot_t slot[ITMM_MAX_TASKS];
static uint8_t count;

static void release(uint8_t n)
{
    for (uint8_t i = 0; i < n; i++) {
        if (slot[i].queue != NULL) {
            vQueueDelete(slot[i].queue);
        }
        if (slot[i].events != NULL) {
            vEventGroupDelete(slot[i].events);
        }
        if (slot[i].reply != NULL) {
            vSemaphoreDelete(slot[i].reply);
        }
    }
    (void)memset(slot, 0, sizeof slot);
}

itmm_status_t itmm_init(const itmm_config_t *cfg)
{
    if (cfg == NULL || count != 0u || cfg->task_count == 0u || cfg->task_count > ITMM_MAX_TASKS) {
        return ITMM_ERR_ARG;
    }
    for (uint8_t i = 0; i < cfg->task_count; i++) {
        if (cfg->queue_depth[i] == 0u) {
            return ITMM_ERR_ARG;
        }
    }
    for (uint8_t i = 0; i < cfg->task_count; i++) {
        slot[i].queue = xQueueCreate(cfg->queue_depth[i], sizeof(itmm_msg_t));
        slot[i].events = xEventGroupCreate();
        slot[i].reply = xSemaphoreCreateBinary();
        if (slot[i].queue == NULL || slot[i].events == NULL || slot[i].reply == NULL) {
            release((uint8_t)(i + 1u));
            return ITMM_ERR_NOMEM;
        }
    }
    count = cfg->task_count;
    return ITMM_OK;
}

static int usable(itmm_task_t task, itmm_events_t mask)
{
    return task < count && mask != 0u && (mask & ~ITMM_EVT_USER_MASK) == 0u;
}

itmm_status_t itmm_event_set(itmm_task_t task, itmm_events_t mask)
{
    if (!usable(task, mask)) {
        return ITMM_ERR_ARG;
    }
    (void)xEventGroupSetBits(slot[task].events, mask);
    return ITMM_OK;
}

itmm_status_t itmm_event_clear(itmm_task_t task, itmm_events_t mask)
{
    if (!usable(task, mask)) {
        return ITMM_ERR_ARG;
    }
    (void)xEventGroupClearBits(slot[task].events, mask);
    return ITMM_OK;
}

itmm_events_t itmm_event_get(itmm_task_t task)
{
    return task < count ? xEventGroupGetBits(slot[task].events) : 0u;
}

itmm_status_t itmm_event_set_from_isr(itmm_task_t task, itmm_events_t mask, BaseType_t *woken)
{
    if (!usable(task, mask)) {
        return ITMM_ERR_ARG;
    }
    return xEventGroupSetBitsFromISR(slot[task].events, mask, woken) == pdPASS ? ITMM_OK
                                                                               : ITMM_ERR_EVENT;
}

itmm_status_t itmm_event_clear_from_isr(itmm_task_t task, itmm_events_t mask)
{
    if (!usable(task, mask)) {
        return ITMM_ERR_ARG;
    }
    return xEventGroupClearBitsFromISR(slot[task].events, mask) == pdPASS ? ITMM_OK
                                                                          : ITMM_ERR_EVENT;
}

itmm_events_t itmm_event_get_from_isr(itmm_task_t task)
{
    return task < count ? xEventGroupGetBitsFromISR(slot[task].events) : 0u;
}

itmm_events_t itmm_wait(itmm_task_t id, itmm_events_t mask, uint32_t options, TickType_t timeout)
{
    if (id >= count || mask == 0u || (mask & ~(ITMM_EVT_USER_MASK | ITMM_EVT_MESSAGE)) != 0u) {
        return 0u;
    }
    int all = (options & ITMM_WAIT_ALL) != 0u;
    /* The kernel must not clear the flags: it would clear the message flag too. */
    itmm_events_t got =
        xEventGroupWaitBits(slot[id].events, mask, pdFALSE, all ? pdTRUE : pdFALSE, timeout) & mask;
    if (all ? got != mask : got == 0u) {
        return 0u; /* the time ran out */
    }
    itmm_events_t clear = got & ITMM_EVT_USER_MASK;
    if ((options & ITMM_NO_CLEAR) == 0u && clear != 0u) {
        /* A flag that is set again after the wait returned and before this call is lost, but
           the task has not looked at its work yet, so it still sees the work of that event. */
        (void)xEventGroupClearBits(slot[id].events, clear);
    }
    return got;
}

static itmm_status_t post(itmm_task_t to, const itmm_msg_t *msg, TickType_t timeout)
{
    if (xQueueSend(slot[to].queue, msg, timeout) != pdTRUE) {
        return ITMM_ERR_FULL;
    }
    (void)xEventGroupSetBits(slot[to].events, ITMM_EVT_MESSAGE);
    return ITMM_OK;
}

itmm_status_t itmm_send(itmm_task_t to, const itmm_msg_t *msg, TickType_t timeout)
{
    if (to >= count || msg == NULL) {
        return ITMM_ERR_ARG;
    }
    itmm_msg_t m = *msg;
    m.token = 0;
    return post(to, &m, timeout);
}

itmm_status_t itmm_send_wait(itmm_task_t to, const itmm_msg_t *msg, TickType_t queue_timeout,
                             TickType_t reply_timeout)
{
    if (to >= count || msg == NULL || msg->sender >= count) {
        return ITMM_ERR_ARG;
    }
    slot_t *me = &slot[msg->sender];
    itmm_msg_t m = *msg;

    (void)xSemaphoreTake(me->reply, 0); /* a reply that came after an earlier timeout */
    if (++me->last == 0u) {
        ++me->last; /* 0 means no message, so skip it when the counter wraps */
    }
    m.token = me->last;
    me->pending = m.token;

    itmm_status_t status = post(to, &m, queue_timeout);
    if (status != ITMM_OK) {
        me->pending = 0;
        return status;
    }
    if (xSemaphoreTake(me->reply, reply_timeout) == pdTRUE) {
        return ITMM_OK;
    }
    /* Time ran out. The receiver may have finished in the same moment: decide under one lock. */
    int finished;
    taskENTER_CRITICAL();
    finished = (me->pending != m.token);
    me->pending = 0;
    taskEXIT_CRITICAL();
    if (finished) {
        (void)xSemaphoreTake(me->reply, 0);
        return ITMM_OK;
    }
    return ITMM_ERR_TIMEOUT;
}

itmm_status_t itmm_send_from_isr(itmm_task_t to, const itmm_msg_t *msg, BaseType_t *woken)
{
    if (to >= count || msg == NULL) {
        return ITMM_ERR_ARG;
    }
    itmm_msg_t m = *msg;
    m.sender = ITMM_FROM_ISR;
    m.token = 0;
    if (xQueueSendFromISR(slot[to].queue, &m, woken) != pdTRUE) {
        return ITMM_ERR_FULL;
    }
    if (xEventGroupSetBitsFromISR(slot[to].events, ITMM_EVT_MESSAGE, woken) != pdPASS) {
        return ITMM_ERR_EVENT; /* the message is queued, and the next message sets the flag */
    }
    return ITMM_OK;
}

itmm_status_t itmm_receive(itmm_task_t self, itmm_msg_t *msg)
{
    if (self >= count || msg == NULL) {
        return ITMM_ERR_ARG;
    }
    if (xQueueReceive(slot[self].queue, msg, 0) == pdTRUE) {
        return ITMM_OK;
    }
    (void)xEventGroupClearBits(slot[self].events, ITMM_EVT_MESSAGE);
    if (uxQueueMessagesWaiting(slot[self].queue) != 0u) {
        /* A sender queued a message between the two calls above. */
        (void)xEventGroupSetBits(slot[self].events, ITMM_EVT_MESSAGE);
    }
    return ITMM_ERR_EMPTY;
}

itmm_status_t itmm_done(const itmm_msg_t *msg)
{
    if (msg == NULL) {
        return ITMM_ERR_ARG;
    }
    if (msg->token == 0u) {
        return ITMM_OK; /* nobody waits for this message */
    }
    if (msg->sender >= count) {
        return ITMM_ERR_ARG;
    }
    slot_t *sender = &slot[msg->sender];
    int waiting;
    taskENTER_CRITICAL();
    waiting = (sender->pending == msg->token);
    if (waiting) {
        sender->pending = 0;
    }
    taskEXIT_CRITICAL();
    if (!waiting) {
        return ITMM_ERR_STALE;
    }
    (void)xSemaphoreGive(sender->reply);
    return ITMM_OK;
}

const char *itmm_status_text(itmm_status_t status)
{
    static const char *const text[] = {"ok",      "bad argument", "queue full",       "queue empty",
                                       "timeout", "stale",        "event not posted", "no memory"};
    return (unsigned)status < sizeof text / sizeof text[0] ? text[status] : "unknown";
}
