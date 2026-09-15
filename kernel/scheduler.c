#include "scheduler.h"

static uint32_t current_index = 0;
static uint32_t thread_current_index = 0;

void scheduler_init(void)
{
    current_index = 0;
    thread_current_index = 0;
}

pcb_t *scheduler_next(void)
{
    pcb_t *table;
    pcb_t *current;
    uint32_t count;
    uint32_t i;
    uint32_t index;

    table = process_get_table();
    count = process_get_count();
    current = process_get_current();

    if (count == 0) {
        return 0;
    }

    /*
     * Stage 1 - Process scheduler
     *
     * If the current process is the last process in the
     * round-robin cycle, return control to the kernel shell.
     */
    if (current != 0 && current_index == 0) {
        current->state = READY;
        process_set_current(0);
        return 0;
    }

    /*
     * A running process becomes READY before selecting
     * the next process.
     */
    if (current != 0 && current->state == RUNNING) {
        current->state = READY;
    }

    /*
     * Round-Robin process selection.
     */
    for (i = 0; i < count; i++) {
        index = (current_index + i) % count;

        if (table[index].state == READY) {
            current_index = (index + 1) % count;

            table[index].state = RUNNING;
            process_set_current(&table[index]);

            return &table[index];
        }
    }

    /*
     * No READY process.
     */
    process_set_current(0);

    return 0;
}


/*
 * Stage 2 - Thread scheduler
 *
 * Select the next READY thread using Round-Robin scheduling.
 * BLOCKED and TERMINATED threads are skipped.
 */
thread_t *scheduler_next_thread(void)
{
    thread_t *table;
    thread_t *current;
    uint32_t count;
    uint32_t i;
    uint32_t index;

    table = thread_get_table();
    count = thread_get_count();
    current = thread_get_current();

    if (count == 0) {
        return 0;
    }

    /*
     * The currently running thread becomes READY
     * before selecting the next thread.
     */
    if (current != 0 && current->state == THREAD_RUNNING) {
        current->state = THREAD_READY;
    }

    /*
     * Search for the next READY thread.
     *
     * BLOCKED threads are skipped until a mutex or
     * semaphore wakes them.
     */
    for (i = 0; i < count; i++) {
        index = (thread_current_index + i) % count;

        if (table[index].state == THREAD_READY) {
            thread_current_index = (index + 1) % count;

            table[index].state = THREAD_RUNNING;
            thread_set_current(&table[index]);

            return &table[index];
        }
    }

    /*
     * No READY thread exists.
     */
    thread_set_current(0);

    return 0;
}
