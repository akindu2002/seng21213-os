/* =============================================================================
 * SENG21213-OS :: Linker Script
 * ============================================================================*/

OUTPUT_FORMAT(elf32-i386)

ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);   /* Read + Execute */
    data PT_LOAD FLAGS(6);   /* Read + Write */
}

SECTIONS {
    . = 0x10000;

    .text : {
        *(.text)
    } :text

    .rodata : {
        *(.rodata*)
    } :text

    .data : {
        *(.data)
    } :data

    .bss : {
        *(COMMON)
        *(.bss)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}
