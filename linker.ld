/* linker.ld */
ENTRY(start)
SECTIONS
{
    . = 0x100000; /* 1MB */
    .text : ALIGN(4K)
    {
        _kernel_start = .;
        *(.multiboot)
        *(.text)
        *(.text.*)
    }
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
    _bss_end = .;
    _kernel_end = .;
}