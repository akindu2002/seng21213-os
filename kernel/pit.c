#include "pit.h"

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_BASE_FREQUENCY 1193182

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pit_init(uint32_t frequency)
{
    uint32_t divisor;

    if (frequency == 0) {
        return;
    }

    divisor = PIT_BASE_FREQUENCY / frequency;

    /* Channel 0, square-wave mode, low byte then high byte. */
    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}
