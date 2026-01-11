section .note.GNU-stack noalloc noexec nowrite progbits
section .text
bits 32
extern timer_interrupt_handler
extern keyboard_handler
extern irq_handler
global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
global load_idt
%macro IRQ 2
global irq%1
irq%1:
    push byte 0
    push byte %2
    jmp irq_common
%endmacro
IRQ 0,  32  ; Таймер
IRQ 1,  33  ; Клавиатура
IRQ 2,  34
IRQ 3,  35  ; COM2
IRQ 4,  36  ; COM1
IRQ 5,  37  ; LPT2
IRQ 6,  38  ; Флоппи
IRQ 7,  39  ; LPT1
IRQ 8,  40  ; RTC
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44  ; PS/2 мышь
IRQ 13, 45  ; пенис
IRQ 14, 46  ; IDE0
IRQ 15, 47  ; IDE1
irq_common:
    pusha
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, [esp + 48]
    push eax
    call irq_handler
    pop eax
    mov al, 0x20
    out 0x20, al
    cmp byte [esp + 48], 40
    jl .master_only
    out 0xA0, al
.master_only:
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret
load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret