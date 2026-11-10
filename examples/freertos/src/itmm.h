#ifndef ITMM_H
#define ITMM_H

#include <stdint.h>

#include "FreeRTOS.h"

/* ITMM, the Inter-Task Message Manager. Every task that takes part has a number, a message queue,
   a group of event flags and a semaphore for blocking messages. The other tasks and the interrupt
   handlers name the receiver by its number and never see a FreeRTOS handle. */

#if configTICK_TYPE_WIDTH_IN_BITS != TICK_TYPE_WIDTH_32_BITS
#error "ITMM needs 24 event bits: use a 32 bit tick type"
#endif

#define ITMM_MAX_TASKS 8u

/* The event flags of a task. Bits 0 to 22 belong to the application. Bit 23 means that the message
   queue of the task is not empty: ITMM sets it when a message arrives and clears it when the last
   message has been taken. The application may wait for it but cannot set or clear it. */
#define ITMM_EVT_USER_MASK 0x007FFFFFu
#define ITMM_EVT_MESSAGE   (1u << 23)

/* The sender number that ITMM puts in a message that an interrupt handler sent. */
#define ITMM_FROM_ISR 0xFFu

/* Options of itmm_wait(). The default is to return when any flag of the mask is set and to clear
   the flags that the wait returned. */
#define ITMM_WAIT_ALL 0x01u /* return only when every flag of the mask is set */
#define ITMM_NO_CLEAR 0x02u /* leave the flags set when the wait returns */

typedef uint8_t itmm_task_t;
typedef uint32_t itmm_events_t;

typedef enum {
    ITMM_OK = 0,
    ITMM_ERR_ARG,     /* a task number, a mask or a pointer that cannot be used */
    ITMM_ERR_FULL,    /* the queue of the receiver has no free slot */
    ITMM_ERR_EMPTY,   /* the queue holds no message */
    ITMM_ERR_TIMEOUT, /* the receiver did not call itmm_done() in time */
    ITMM_ERR_STALE,   /* itmm_done() came after the sender stopped waiting */
    ITMM_ERR_EVENT,   /* from an interrupt: the message is queued but the event was not posted */
    ITMM_ERR_NOMEM    /* itmm_init() could not allocate */
} itmm_status_t;

/* A message is copied into the queue of the receiver. A pointer in data stays valid for the
   receiver only if the sender waits in itmm_send_wait(), because that call does not return before
   the receiver calls itmm_done(). */
typedef struct {
    uint8_t sender; /* set by the caller of itmm_send(), by ITMM for an interrupt */
    uint8_t type;   /* the application defines the types */
    uint16_t token; /* set by ITMM: not zero for a message that a sender waits for */
    uint32_t value;
    const void *data;
} itmm_msg_t;

typedef struct {
    uint8_t task_count;                  /* 1 to ITMM_MAX_TASKS */
    uint8_t queue_depth[ITMM_MAX_TASKS]; /* messages that each task can hold, 1 or more */
} itmm_config_t;

/* Creates the queues, the event groups and the semaphores. Call once, before the tasks that use
   ITMM run. */
itmm_status_t itmm_init(const itmm_config_t *cfg);

/* Event flags, called from a task. The mask holds application flags only. */
itmm_status_t itmm_event_set(itmm_task_t task, itmm_events_t mask);
itmm_status_t itmm_event_clear(itmm_task_t task, itmm_events_t mask);
itmm_events_t itmm_event_get(itmm_task_t task);

/* The same from an interrupt handler. FreeRTOS hands the work to the timer task, so the flags
   change after the handler returns, and ITMM_ERR_EVENT means that the timer queue was full. */
itmm_status_t itmm_event_set_from_isr(itmm_task_t task, itmm_events_t mask, BaseType_t *woken);
itmm_status_t itmm_event_clear_from_isr(itmm_task_t task, itmm_events_t mask);
itmm_events_t itmm_event_get_from_isr(itmm_task_t task);

/* Waits in the task with this number until a flag of the mask is set (all of them with
   ITMM_WAIT_ALL). The mask may include ITMM_EVT_MESSAGE. Returns the flags of the mask that were
   set, or 0 when the time ran out. The application flags that it returns are cleared, unless
   ITMM_NO_CLEAR is set. The message flag is never cleared here, only by itmm_receive(). */
itmm_events_t itmm_wait(itmm_task_t id, itmm_events_t mask, uint32_t options, TickType_t timeout);

/* Sends a message to a task and returns when it is in the queue. Waits at most timeout ticks for a
   free slot. */
itmm_status_t itmm_send(itmm_task_t to, const itmm_msg_t *msg, TickType_t timeout);

/* Sends a message and then waits until the receiver calls itmm_done() for it. Each task can have
   one such message outstanding, so msg->sender must be the number of the calling task. */
itmm_status_t itmm_send_wait(itmm_task_t to, const itmm_msg_t *msg, TickType_t queue_timeout,
                             TickType_t reply_timeout);

/* From an interrupt handler: does not wait. ITMM_ERR_FULL means the message was dropped. */
itmm_status_t itmm_send_from_isr(itmm_task_t to, const itmm_msg_t *msg, BaseType_t *woken);

/* Takes one message from the queue of the task, without waiting. Returns ITMM_ERR_EMPTY when there
   is none, and clears the message flag at that point. Call it until it returns ITMM_ERR_EMPTY. */
itmm_status_t itmm_receive(itmm_task_t self, itmm_msg_t *msg);

/* Tells the sender of a blocking message that the receiver is finished with it. For a message that
   nobody waits for it does nothing and returns ITMM_OK. Returns ITMM_ERR_STALE when the sender has
   given up already. */
itmm_status_t itmm_done(const itmm_msg_t *msg);

const char *itmm_status_text(itmm_status_t status);

#endif
