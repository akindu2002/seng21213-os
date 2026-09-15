; =============================================================================
; SENG21213-OS :: Process Context Switch
; Stage 1 - Process Management
; Lecture 9 - Process Management
; =============================================================================

BITS 32

global switch_context

; void switch_context(uint32_t *old_esp, uint32_t new_esp);
;
; old_esp = address where current ESP is saved
; new_esp = ESP of the next process

switch_context:
    ; Save all general-purpose registers
    pushad

    ; Save current ESP
    mov eax, [esp + 36]
    mov [eax], esp

    ; Load next process ESP
    mov esp, [esp + 40]

    ; Restore next process registers
    popad

    ; Continue execution of the next process
    ret
