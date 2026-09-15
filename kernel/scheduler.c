#include "scheduler.h"

static uint32_t current_index = 0;

void scheduler_init(void)
{
    current_index = 0;
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
     * If the current process is the last process in the
     * round-robin cycle, give the CPU back to the kernel shell.
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
     * Round-Robin selection.
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
     * Return to the kernel shell.
     */
    process_set_current(0);

    return 0;
}

