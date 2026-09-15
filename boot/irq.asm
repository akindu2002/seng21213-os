; =============================================================================
; SENG21213-OS :: IRQ0 Handler
; Stage 1 - Round-Robin Scheduler
; Lecture 9 - Process Management
; =============================================================================

BITS 32

extern irq0_handler_c
global irq0_handler

irq0_handler:
    ; Save CPU registers
    pushad

    ; Send End Of Interrupt (EOI) to the master PIC
    mov al, 0x20
    out 0x20, al

    ; Call the C IRQ0 handler
    call irq0_handler_c

    ; Restore CPU registers
    popad

    ; Return from interrupt
    iretd
