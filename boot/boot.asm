;==== Multiboot Header ====
section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)
;==== Основной код ====
section .text
bits 32
global start
extern kernel_main
extern keyboard_handler
start:
    cli
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx
    cmp eax, 0x2BADB002
    jne .invalid_multiboot
    mov esp, stack_top
    call clear_bss
    call enable_a20
    call init_gdt
    call init_idt
    call init_pic
    sti
    mov esi, boot_msg
    call early_print
    push dword [multiboot_info]
    push dword [multiboot_magic]
    call kernel_main
    add esp, 8
.hang:
    cli
    hlt
    jmp .hang
.invalid_multiboot:
    mov esi, multiboot_error_msg
    call early_print
    jmp .hang
clear_bss:
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    ret
enable_a20:
    call .wait_input
    mov al, 0xAD
    out 0x64, al
    call .wait_input
    mov al, 0xD0
    out 0x64, al
    call .wait_output
    in al, 0x60
    push eax
    call .wait_input
    mov al, 0xD1
    out 0x64, al
    call .wait_input
    pop eax
    or al, 2
    out 0x60, al
    call .wait_input
    mov al, 0xAE
    out 0x64, al
    call .wait_input
    ret
.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret
.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret
init_gdt:
    lgdt [gdt_descriptor]
    jmp 0x08:.reload_cs
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
init_idt:
    mov edi, idt
    mov ecx, 256
    xor eax, eax
    rep stosd
    mov eax, keyboard_isr
    mov word [idt + 0x21 * 8], ax
    mov word [idt + 0x21 * 8 + 2], 0x08
    mov word [idt + 0x21 * 8 + 4], 0x8E00
    shr eax, 16
    mov word [idt + 0x21 * 8 + 6], ax
    lidt [idt_descriptor]
    ret
init_pic:
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    mov al, 0xFC
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al
    ret
keyboard_isr:
    pusha
    cld
    in al, 0x60
    movzx eax, al
    push eax
    call keyboard_handler
    add esp, 4
    mov al, 0x20
    out 0x20, al
    popa
    iret
early_print:
    mov edi, 0xB8000
    mov ah, 0x0F
.print_loop:
    lodsb
    test al, al
    jz .print_done
    stosw
    jmp .print_loop
.print_done:
    ret
;==== Данные ====
section .data
align 4
boot_msg: db "Bootloader: Starting kernel...", 0
multiboot_error_msg: db "ERROR: Invalid Multiboot signature", 0
gdt:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt
idt_descriptor:
    dw 256 * 8 - 1
    dd idt
;==== BSS секция ====
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
global multiboot_info
global multiboot_magic
multiboot_info:
    resd 1
multiboot_magic:
    resd 1
align 8
idt:
    resb 256 * 8
global bss_start
global bss_end
bss_start:
bss_end:
section .note.GNU-stack noalloc noexec nowrite progbits