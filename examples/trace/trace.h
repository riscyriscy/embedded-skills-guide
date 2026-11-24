#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

/* A trace is a ring of small records in RAM: what happened (an id) and when (a
   timestamp from a free running counter). Recording is lock free, so it works from
   the main loop and from interrupts, and it costs a few instructions. The records
   leave the target after the run, or on request. */
enum trace_id {
    TR_ISR_ENTER = 1,
    TR_ISR_EXIT,
    TR_SAMPLE_BEGIN,
    TR_SAMPLE_END,
    TR_LOG_BEGIN,
    TR_LOG_END,
};

struct trace_record {
    uint32_t time; /* counter clocks, counting up */
    uint32_t id;
};

void trace_init(void);
void trace_event(enum trace_id id);
/* Writes "T <time> <id>" for every record, oldest first, with plat_puts. */
void trace_dump(void);

#endif
