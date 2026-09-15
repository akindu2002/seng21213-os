#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "../include/types.h"
#include "thread.h"

#define SEM_MAX_WAITERS MAX_THREADS

typedef struct {
    volatile uint32_t count;

    /* FIFO queue of threads waiting for this semaphore */
    thread_t *wait_queue[SEM_MAX_WAITERS];
    uint32_t wait_head;
    uint32_t wait_tail;
    uint32_t wait_count;
} semaphore_t;

void sem_init(semaphore_t *sem, uint32_t value);
void sem_wait(semaphore_t *sem);
void sem_signal(semaphore_t *sem);

#endif
