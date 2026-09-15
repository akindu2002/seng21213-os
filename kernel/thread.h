#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"

#define MAX_THREADS 16
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

typedef struct thread {
    uint32_t tid;
    thread_state_t state;
    uint32_t esp;
    uint32_t eip;
    void *arg;
    uint8_t stack[THREAD_STACK_SIZE];
} thread_t;

void thread_init(void);
thread_t *thread_create(void (*fn)(void *), void *arg);

thread_t *thread_get_table(void);
uint32_t thread_get_count(void);

void thread_set_current(thread_t *thread);
thread_t *thread_get_current(void);

/* Stage 2 - Thread scheduling support */
void thread_yield(void);
void thread_block(void);
void thread_unblock(thread_t *thread);

#endif
