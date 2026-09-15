#include "idt.h"

#define IDT_ENTRIES 256
#define KERNEL_CODE_SEGMENT 0x08

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ __volatile__(
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void pic_remap(void)
{
    uint8_t master_mask;
    uint8_t slave_mask;

    master_mask = inb(0x21);
    slave_mask = inb(0xA1);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    /*
     * Only IRQ0 (PIT timer) is enabled.
     * IRQ1 is masked because the keyboard driver uses polling.
     */
    (void)master_mask;
    (void)slave_mask;

    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

extern void irq0_handler(void);

static void idt_set_gate(uint8_t vector, uint32_t handler)
{
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector = KERNEL_CODE_SEGMENT;
    idt[vector].zero = 0;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

void idt_init(void)
{
    uint32_t i;

    for (i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }

    pic_remap();

    /* IRQ0 is mapped to interrupt vector 32 after PIC remapping. */
    idt_set_gate(32, (uint32_t)irq0_handler);

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;

    __asm__ __volatile__(
        "lidtl %0"
        :
        : "m"(idt_ptr)
    );
}
