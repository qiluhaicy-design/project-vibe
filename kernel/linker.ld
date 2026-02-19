ENTRY(_start)

MEMORY {
    ram (rwx) : ORIGIN = 0x8000, LENGTH = 0x1000000
}

SECTIONS {
    .text : {
        *(.text*)
    } > ram
    .data : {
        *(.data*)
    } > ram
    .bss : {
        *(.bss*)
    } > ram
}