#include "process.h"

static pcb_t process_table[MAX_PROCESSES];
static uint32_t process_count = 0;
static uint32_t next_pid = 1;
static pcb_t *current_process = 0;

static void process_bootstrap(void)
{
    void (*entry)(void);

    __asm__ __volatile__("sti");

    entry = (void (*)(void))current_process->eip;

    if (entry != 0) {
        entry();
    }

    process_exit();

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

void process_init(void)
{
    uint32_t i;

    process_count = 0;
    next_pid = 1;
    current_process = 0;

    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = TERMINATED;
        process_table[i].esp = 0;
        process_table[i].eip = 0;
    }
}

pcb_t *process_create(void (*entry)(void))
{
    pcb_t *process;
    uint32_t *sp;
    uint32_t i;

    if (process_count >= MAX_PROCESSES || entry == 0) {
        return 0;
    }

    process = &process_table[process_count];

    process->pid = next_pid++;
    process->state = READY;
    process->eip = (uint32_t)entry;

    /*
     * Build the initial stack expected by switch_context():
     *
     *   POPAD -> restores 8 registers
     *   RET   -> jumps to process_bootstrap()
     */
    sp = (uint32_t *)&process->stack[STACK_SIZE];

    *--sp = (uint32_t)process_bootstrap;

    for (i = 0; i < 8; i++) {
        *--sp = 0;
    }

    process->esp = (uint32_t)sp;

    process_count++;

    return process;
}

pcb_t *process_get_table(void)
{
    return process_table;
}

uint32_t process_get_count(void)
{
    return process_count;
}

pcb_t *process_get_current(void)
{
    return current_process;
}

void process_set_current(pcb_t *process)
{
    current_process = process;
}

void process_exit(void)
{
    if (current_process != 0) {
        current_process->state = TERMINATED;
        current_process = 0;
    }
}
