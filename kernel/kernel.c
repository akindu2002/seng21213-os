/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "scheduler.h"
#include "pit.h"
#include "idt.h"
#include "../include/types.h"


/* Stage 1 - Context switching */
extern void switch_context(uint32_t *old_esp, uint32_t new_esp);

static uint32_t kernel_esp = 0;

/*
 * IRQ0 scheduler entry.
 *
 * The assembly IRQ handler passes the address of the saved
 * register frame. This function returns the ESP of the
 * context that should continue after the interrupt.
 */
uint32_t irq0_handler_c(uint32_t current_esp)
{
    thread_t *current_thread;
    thread_t *next_thread;

    /*
     * Stage 2 - Thread scheduling.
     */
    if (thread_get_count() > 0) {
        current_thread = thread_get_current();

        /*
         * Timer interrupted the kernel shell.
         * Remember the shell's interrupt context.
         */
        if (current_thread == 0) {
            kernel_esp = current_esp;
        } else {
            /*
             * Save the interrupted thread's context.
             */
            current_thread->esp = current_esp;
        }

        next_thread = scheduler_next_thread();

        if (next_thread != 0) {
            return next_thread->esp;
        }

        /*
         * No READY thread.
         * Return to the kernel shell if its context exists.
         */
        if (kernel_esp != 0) {
            thread_set_current(0);
            return kernel_esp;
        }

        return current_esp;
    }

    /*
     * Stage 1 fallback.
     *
     * When Stage 2 threads do not exist, keep the current
     * interrupt context. Stage 1 process switching will be
     * re-integrated after the thread context switch is stable.
     */
    return current_esp;
}


/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);

/* ---------------------------------------------------------------------------
 * Stage 1: Test processes
 * --------------------------------------------------------------------------*/
static void test_process_1(void)
{
    volatile uint32_t i;
    uint32_t counter = 0;

    for (;;) {
        counter++;


        for (i = 0; i < 1000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

static void test_process_2(void)
{
    volatile uint32_t i;
    uint32_t counter = 0;

    for (;;) {
        counter++;


        for (i = 0; i < 1000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

/* ---------------------------------------------------------------------------
 * Stage 2: Mutex / Race Condition Test
 * --------------------------------------------------------------------------*/

static volatile uint32_t myglobal = 0;
static mutex_t test_mutex;

/* Stage 2 - Bounded Buffer Producer/Consumer */
#define BUFFER_SIZE 8

static int buffer[BUFFER_SIZE];
static uint32_t buffer_in = 0;
static uint32_t buffer_out = 0;

static semaphore_t empty_slots;
static semaphore_t full_slots;
static semaphore_t buffer_mutex;

/* ---------------------------------------------------------------------------
 * Stage 2: Test threads
 * --------------------------------------------------------------------------*/


static void test_thread_1(void *arg)
{
    uint32_t i;

    (void)arg;

    vga_puts("\n[THREAD 1] Mutex test started\n");

    for (i = 0; i < 10; i++) {
        mutex_lock(&test_mutex);

        myglobal++;

        vga_puts("[THREAD 1] myglobal = ");
        vga_printf("%u", myglobal);
        vga_puts("\n");

        mutex_unlock(&test_mutex);

        for (uint32_t delay = 0; delay < 1000; delay++) {
            __asm__ __volatile__("nop");
        }
    }

    vga_puts("[THREAD 1] Finished\n");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

static void test_thread_2(void *arg)
{
    uint32_t i;

    (void)arg;

    vga_puts("\n[THREAD 2] Mutex test started\n");

    for (i = 0; i < 10; i++) {
        mutex_lock(&test_mutex);

        myglobal++;

        vga_puts("[THREAD 2] myglobal = ");
        vga_printf("%u", myglobal);
        vga_puts("\n");

        mutex_unlock(&test_mutex);

        for (uint32_t delay = 0; delay < 1000; delay++) {
            __asm__ __volatile__("nop");
        }
    }

    vga_puts("[THREAD 2] Finished\n");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

/* ---------------------------------------------------------------------------
 * Stage 2: Bounded Buffer Producer
 * --------------------------------------------------------------------------*/

static void producer_thread(void *arg)
{
    int item;
    uint32_t i;

    (void)arg;

    vga_puts("\n[PRODUCER] Started\n");

    for (item = 1; item <= 20; item++) {
        /* Wait until the buffer has an empty slot. */
        sem_wait(&empty_slots);

        /* Only one thread may modify the buffer at a time. */
        sem_wait(&buffer_mutex);

        buffer[buffer_in] = item;
        buffer_in = (buffer_in + 1) % BUFFER_SIZE;

        vga_puts("[PRODUCER] Produced item ");
        vga_printf("%u", (uint32_t)item);
        vga_puts("\n");

        sem_signal(&buffer_mutex);

        /* Tell the consumer that one item is available. */
        sem_signal(&full_slots);

        for (i = 0; i < 2000; i++) {
            __asm__ __volatile__("nop");
        }
    }

    vga_puts("[PRODUCER] Finished\n");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

/* ---------------------------------------------------------------------------
 * Stage 2: Bounded Buffer Consumer
 * --------------------------------------------------------------------------*/

static void consumer_thread(void *arg)
{
    int item;
    uint32_t i;

    (void)arg;

    vga_puts("\n[CONSUMER] Started\n");

    for (i = 0; i < 20; i++) {
        /* Wait until the buffer contains an item. */
        sem_wait(&full_slots);

        /* Only one thread may modify the buffer at a time. */
        sem_wait(&buffer_mutex);

        item = buffer[buffer_out];
        buffer_out = (buffer_out + 1) % BUFFER_SIZE;

        vga_puts("[CONSUMER] Consumed item ");
        vga_printf("%u", (uint32_t)item);
        vga_puts("\n");

        sem_signal(&buffer_mutex);

        /* One more empty slot is now available. */
        sem_signal(&empty_slots);

        for (uint32_t delay = 0; delay < 3000; delay++) {
            __asm__ __volatile__("nop");
        }
    }

    vga_puts("[CONSUMER] Finished\n");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help    – Show this help message\n");
    vga_puts("  clear   – Clear the screen\n");
    vga_puts("  about   – About this OS and course\n");
    vga_puts("  echo    – Echo text to screen\n");
    vga_puts("  mem     – Memory map (stub)\n");
    vga_puts_color("\n  Milestones (to implement):\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ps      – [L09] List processes\n");
    vga_puts("  kill    – [L09] Terminate a process\n");
    vga_puts("  threads – [L10] List kernel threads\n");
    vga_puts("  free    – [L11] Show free memory\n");
    vga_puts("  ls      – [L12] List files\n");
    vga_puts("  cat     – [L12] Print file contents\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  About SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Architecture : x86 (i686), 32-bit Protected Mode\n");
    vga_puts("  Bootloader   : Custom MBR (NASM)\n");
    vga_puts("  Kernel       : Freestanding C (GCC, no libc)\n");
    vga_puts("  VM Target    : QEMU (qemu-system-i386)\n");
    vga_puts("  Course       : SENG 21213 – Sem 2\n");
    vga_puts("  Reference    : Stallings, OS: Internals & Design Principles\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    /* Stage 0 stub – students implement the real PMM in Lecture 11 */
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF  :  First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF  :  Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF  :  BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF     :  VGA frame buffer\n");
    vga_puts_color("\n  TODO: Use BIOS int 0x15, EAX=0xE820 to get real memory map\n\n",
                   VGA_YELLOW, VGA_BLACK);
}

/* ---------------------------------------------------------------------------
 * Stage 1: Process list
 * --------------------------------------------------------------------------*/
static void cmd_ps(void)
{
    pcb_t *table;
    uint32_t count;
    uint32_t i;

    table = process_get_table();
    count = process_get_count();

    vga_puts("PID   STATE\n");
    vga_puts("----------------\n");

    for (i = 0; i < count; i++) {
        vga_printf("%u   ", table[i].pid);

        switch (table[i].state) {
            case READY:
                vga_puts("READY");
                break;

            case RUNNING:
                vga_puts("RUNNING");
                break;

            case BLOCKED:
                vga_puts("BLOCKED");
                break;

            case TERMINATED:
                vga_puts("TERMINATED");
                break;

            default:
                vga_puts("UNKNOWN");
                break;
        }

        vga_puts("\n");
    }
}


/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        /* Stage 1: Process management */
        if (k_strcmp(cmd, "ps") == 0) {
            cmd_ps();
            continue;
        }

        /* Milestone stubs */
        if (k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();

    /* Stage 1 - Process Management (Lecture 09) */
    process_init();
    thread_init();
    scheduler_init();

    /* Stage 2 - Initialize mutex */
    mutex_init(&test_mutex);

    /* Stage 2 - Initialize producer-consumer semaphores */
    sem_init(&empty_slots, BUFFER_SIZE);
    sem_init(&full_slots, 0);
    sem_init(&buffer_mutex, 1);

    /* Stage 1 - Interrupts and timer */
    idt_init();
    pit_init(100);

    /* Stage 1 - Create two test processes */
    process_create(test_process_1);
    process_create(test_process_2);

    /* Stage 2 - Mutex test threads temporarily disabled */
    thread_create(test_thread_1, 0);
    thread_create(test_thread_2, 0);

    /* Stage 2 - Create producer and consumer threads */
    thread_create(producer_thread, 0);
    thread_create(consumer_thread, 0);

    print_splash();

    /* Enable hardware interrupts after the splash screen */
    __asm__ __volatile__("sti");
    shell_run();

    /* Should never reach here */
    __asm__ __volatile__("hlt");
}
