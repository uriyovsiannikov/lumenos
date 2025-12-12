; SPDX-License-Identifier: GPL-3.0-or-later */
section .note.GNU-stack noalloc noexec nowrite progbits
section .text
bits 32
extern timer_interrupt_handler
extern keyboard_handler
global irq0
global irq1
global load_idt
; IRQ0 - Прерывание таймера
irq0:
    pushad
    call timer_interrupt_handler
    mov al, 0x20
    out 0x20, al
    popad
    iret
; IRQ1 - Прерывание клавиатуры
irq1:
    pushad
    call keyboard_handler
    mov al, 0x20
    out 0x20, al
    popad
    iret
load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret