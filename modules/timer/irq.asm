; ======================
; irq.asm
; Обработчики аппаратных прерываний
; ======================
section .note.GNU-stack noalloc noexec nowrite progbits
section .text
bits 32

; ===== Внешние функции =====
extern timer_interrupt_handler  ; Из timer.c
extern keyboard_handler         ; Из kernel.c

; ===== Глобальные метки =====
global irq0
global irq1
global load_idt

; ===== Обработчики прерываний =====

; IRQ0 - Прерывание таймера
irq0:
    pushad                     ; Сохраняем все регистры
    call timer_interrupt_handler
    mov al, 0x20               ; Отправляем EOI (End Of Interrupt)
    out 0x20, al
    popad                      ; Восстанавливаем регистры
    iret

; IRQ1 - Прерывание клавиатуры
irq1:
    pushad
    call keyboard_handler
    mov al, 0x20
    out 0x20, al
    popad
    iret

; ===== Загрузка IDT =====
load_idt:
    mov edx, [esp + 4]         ; Получаем указатель на IDT
    lidt [edx]                 ; Загружаем IDT
    ret