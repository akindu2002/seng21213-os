[BITS 16]
[ORG 0x8000]

start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    ; -------------------------------------------------
    ; BIOS E820 memory map
    ; Store entries at physical address 0x9000.
    ; Store entry count at 0x8FF0.
    ; Each entry is 24 bytes.
    ; -------------------------------------------------
    xor ebx, ebx
    mov di, 0x9000
    xor bp, bp

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15

    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done

    add di, 24
    inc bp

    test ebx, ebx
    jnz .e820_loop

.e820_done:
    mov [0x8FF0], bp

    ; -------------------------------------------------
    ; Load kernel from disk.
    ; Stage 2 occupies sector 2.
    ; Kernel starts at sector 3.
    ; Load 64 sectors to physical 0x10000.
    ; -------------------------------------------------
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 3
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13

    jc disk_error

    ; -------------------------------------------------
    ; Enter protected mode
    ; -------------------------------------------------
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm32

disk_error:
    mov si, disk_error_msg

.print:
    lodsb
    test al, al
    jz $
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print

[BITS 32]
init_pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov esp, 0x180000

    call 0x10000

.halt:
    cli
    hlt
    jmp .halt

boot_drive db 0

disk_error_msg db 'Disk read error', 0

gdt_start:
    dq 0x0000000000000000

    ; Code segment
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    ; Data segment
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

times 510 - ($ - $$) db 0
dw 0xAA55
