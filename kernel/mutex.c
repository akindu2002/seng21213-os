#include "mutex.h"
#include "thread.h"

/*
 * Stage 2 - Blocking mutex
 *
 * Interrupts are disabled while the mutex state and wait queue
 * are modified so that a timer interrupt cannot change the
 * current thread between checking and updating the mutex.
 */

void mutex_init(mutex_t *mutex)
{
    uint32_t i;

    if (mutex == 0) {
        return;
    }

    mutex->locked = 0;
    mutex->owner = 0;
    mutex->wait_head = 0;
    mutex->wait_tail = 0;
    mutex->wait_count = 0;

    for (i = 0; i < MUTEX_MAX_WAITERS; i++) {
        mutex->wait_queue[i] = 0;
    }
}

void mutex_lock(mutex_t *mutex)
{
    thread_t *current;

    if (mutex == 0) {
        return;
    }

    current = thread_get_current();

    /*
     * Protect the check-and-acquire operation from the timer IRQ.
     */
    __asm__ __volatile__("cli");

    /*
     * Already the owner.
     * This also allows a woken waiter that was transferred
     * ownership by mutex_unlock() to finish mutex_lock().
     */
    if (current != 0 && mutex->owner == current) {
        __asm__ __volatile__("sti");
        return;
    }

    /*
     * Mutex is free: acquire it immediately.
     */
    if (mutex->locked == 0) {
        mutex->locked = 1;
        mutex->owner = current;

        __asm__ __volatile__("sti");
        return;
    }

    /*
     * Mutex is busy.
     * Put the current thread into the FIFO wait queue.
     */
    if (current != 0 &&
        mutex->wait_count < MUTEX_MAX_WAITERS) {

        mutex->wait_queue[mutex->wait_tail] = current;
        mutex->wait_tail =
            (mutex->wait_tail + 1) % MUTEX_MAX_WAITERS;
        mutex->wait_count++;

        current->state = THREAD_BLOCKED;

        /*
         * Allow the timer interrupt to run. The scheduler will
         * skip this BLOCKED thread and select another READY thread.
         */
        __asm__ __volatile__("sti");

        /*
         * Sleep until the scheduler runs this thread again.
         */
        for (;;) {
            __asm__ __volatile__("hlt");

            /*
             * mutex_unlock() transfers ownership to the waiting
             * thread before making it READY.
             */
            if (mutex->owner == current) {
                return;
            }
        }
    }

    /*
     * No current thread or wait queue is full.
     * Restore interrupts without changing the mutex.
     */
    __asm__ __volatile__("sti");
}

void mutex_unlock(mutex_t *mutex)
{
    thread_t *current;
    thread_t *next;

    if (mutex == 0) {
        return;
    }

    current = thread_get_current();

    __asm__ __volatile__("cli");

    /*
     * Only the owner may unlock the mutex.
     */
    if (current != 0 && mutex->owner != current) {
        __asm__ __volatile__("sti");
        return;
    }

    /*
     * If another thread is waiting, transfer ownership directly
     * to the first waiter.
     */
    if (mutex->wait_count > 0) {
        next = mutex->wait_queue[mutex->wait_head];

        mutex->wait_queue[mutex->wait_head] = 0;
        mutex->wait_head =
            (mutex->wait_head + 1) % MUTEX_MAX_WAITERS;
        mutex->wait_count--;

        mutex->owner = next;
        mutex->locked = 1;

        thread_unblock(next);
    } else {
        /*
         * Nobody is waiting, so the mutex becomes free.
         */
        mutex->owner = 0;
        mutex->locked = 0;
    }

    __asm__ __volatile__("sti");
}
