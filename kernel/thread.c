#include "thread.h"

static thread_t thread_table[MAX_THREADS];
static uint32_t thread_count = 0;
static thread_t *current_thread = 0;
static uint32_t next_tid = 1;


/*
 * Stage 2 - Thread entry wrapper.
 *
 * A newly created thread starts here after its context
 * is restored by switch_context().
 */
static void thread_bootstrap(void)
{
    void (*fn)(void *);
    thread_t *thread;

    thread = current_thread;

    if (thread != 0) {
        fn = (void (*)(void *))thread->eip;

        if (fn != 0) {
            fn(thread->arg);
        }

        /*
         * The thread function has finished.
         * Mark this thread as terminated.
         */
        thread->state = THREAD_TERMINATED;
    }

    /*
    * The thread function has finished.
    * Keep interrupts enabled and wait for the
    * scheduler to switch back to the kernel.
    */
    for (;;) {
        __asm__ __volatile__("sti");
        __asm__ __volatile__("hlt");
    }
}

void thread_init(void)
{
    uint32_t i;

    thread_count = 0;
    current_thread = 0;
    next_tid = 1;

    for (i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_TERMINATED;
        thread_table[i].esp = 0;
        thread_table[i].eip = 0;
        thread_table[i].arg = 0;
    }
}

thread_t *thread_create(void (*fn)(void *), void *arg)
{
    thread_t *thread;
    uint32_t *sp;
    uint32_t i;

    if (thread_count >= MAX_THREADS || fn == 0) {
        return 0;
    }

    thread = &thread_table[thread_count];

    thread->tid = next_tid++;
    thread->state = THREAD_READY;
    thread->eip = (uint32_t)fn;
    thread->arg = arg;

    /*
     * Build the initial interrupt context expected by irq.asm.
     *
     * irq.asm restores the context using:
     *
     *     popad
     *     iretd
     *
     * Therefore the initial stack must contain:
     *
     *   8 saved registers for POPAD
     *   EIP
     *   CS
     *   EFLAGS
     *
     * The thread starts at thread_bootstrap().
     */
    sp = (uint32_t *)&thread->stack[THREAD_STACK_SIZE];

    /* IRET frame: EFLAGS, CS, EIP */
    *--sp = 0x00000202;
    *--sp = 0x00000008;
    *--sp = (uint32_t)thread_bootstrap;

    /* POPAD frame: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX */
    for (i = 0; i < 8; i++) {
        *--sp = 0;
    }

    thread->esp = (uint32_t)sp;

    thread_count++;

    return thread;
}

thread_t *thread_get_table(void)
{
    return thread_table;
}

uint32_t thread_get_count(void)
{
    return thread_count;
}

void thread_set_current(thread_t *thread)
{
    current_thread = thread;
}

thread_t *thread_get_current(void)
{
    return current_thread;
}

/*
 * Stage 2 - Cooperative thread state helpers.
 *
 * Actual context switching is performed by the scheduler
 * through the timer interrupt.
 */
void thread_yield(void)
{
    /*
     * A running thread becomes READY so that the scheduler
     * can select another READY thread.
     */
    if (current_thread != 0 &&
        current_thread->state == THREAD_RUNNING) {
        current_thread->state = THREAD_READY;
    }
}

void thread_block(void)
{
    if (current_thread != 0) {
        current_thread->state = THREAD_BLOCKED;
    }
}

void thread_unblock(thread_t *thread)
{
    if (thread != 0 &&
        thread->state == THREAD_BLOCKED) {
        thread->state = THREAD_READY;
    }
}
