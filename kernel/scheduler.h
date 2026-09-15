#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "thread.h"

void scheduler_init(void);

/* Stage 1 - Process scheduler */
pcb_t *scheduler_next(void);

/* Stage 2 - Thread scheduler */
thread_t *scheduler_next_thread(void);

#endif
