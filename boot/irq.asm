; =============================================================================
; SENG21213-OS :: IRQ0 Handler
; Stage 2 - Thread Context Switching
; Lecture 10 - Threads and Synchronisation
; =============================================================================

BITS 32

extern irq0_handler_c
global irq0_handler

irq0_handler:
    ; Save all general-purpose registers.
    ; ESP now points to the saved-register frame.
    pushad

    ; Send End Of Interrupt (EOI) to the master PIC.
    mov al, 0x20
    out 0x20, al

    ; Pass the current saved ESP to the C scheduler.
    push esp
    call irq0_handler_c
    add esp, 4

    ; C returns the ESP of the context that should continue.
    ; EAX == 0 means keep the current context.
    test eax, eax
    jz .restore_current

    ; Switch to the selected context.
    mov esp, eax

.restore_current:
    ; Restore registers from the selected context.
    popad

    ; Return from the interrupt.
    iretd
