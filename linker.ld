ENTRY(start)

SECTIONS {
    . = 1M;                     /* Загрузка по адресу 1MB */
    
    .multiboot : ALIGN(4) {     /* Multiboot заголовок в начале! */
        KEEP(*(.multiboot))
    }
    
    .text : ALIGN(4K) {
        *(.text)
        *(.text.*)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : ALIGN(4K) {
        *(.data)
        *(.data.*)
    }
    
    .bss : ALIGN(4K) {
        bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        bss_end = .;
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
    
    kernel_end = .;
}