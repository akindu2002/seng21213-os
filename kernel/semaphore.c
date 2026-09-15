#include "semaphore.h"

void sem_init(semaphore_t *sem, uint32_t value)
{
    uint32_t i;

    if (sem == 0) {
        return;
    }

    sem->count = value;
    sem->wait_head = 0;
    sem->wait_tail = 0;
    sem->wait_count = 0;

    for (i = 0; i < SEM_MAX_WAITERS; i++) {
        sem->wait_queue[i] = 0;
    }
}

void sem_wait(semaphore_t *sem)
{
    thread_t *current;

    if (sem == 0) {
        return;
    }

    current = thread_get_current();

    __asm__ __volatile__("cli");

    if (sem->count > 0) {
        sem->count--;
        __asm__ __volatile__("sti");
        return;
    }

    if (current != 0 &&
        sem->wait_count < SEM_MAX_WAITERS) {

        sem->wait_queue[sem->wait_tail] = current;
        sem->wait_tail =
            (sem->wait_tail + 1) % SEM_MAX_WAITERS;
        sem->wait_count++;

        current->state = THREAD_BLOCKED;

        __asm__ __volatile__("sti");

        for (;;) {
            __asm__ __volatile__("hlt");

            if (current->state != THREAD_BLOCKED) {
                return;
            }
        }
    }

    __asm__ __volatile__("sti");
}

void sem_signal(semaphore_t *sem)
{
    thread_t *next;

    if (sem == 0) {
        return;
    }

    __asm__ __volatile__("cli");

    if (sem->wait_count > 0) {
        next = sem->wait_queue[sem->wait_head];

        sem->wait_queue[sem->wait_head] = 0;
        sem->wait_head =
            (sem->wait_head + 1) % SEM_MAX_WAITERS;
        sem->wait_count--;

        thread_unblock(next);
    } else {
        sem->count++;
    }

    __asm__ __volatile__("sti");
}
