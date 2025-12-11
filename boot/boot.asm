;==== BOOTBOOT Header ====
section .bootboot
align 4096

bootboot_header:
    db 'B', 'O', 'O', 'T'   ; magic
    dd bootboot_header_end - bootboot_header ; size
    db 1                    ; protocol version
    db 0                    ; framebuffer_type (0 = text mode)
    dw 80                   ; width (text columns)
    dw 25                   ; height (text rows)
    dw 0                    ; bpp (0 for text mode)
    dw 0                    ; pitch
    dq 0                    ; fb_ptr
    dq 0                    ; fb_size
    dq 0                    ; initrd_ptr
    dq 0                    ; initrd_size
    dq 0                    ; acpi_ptr
    dq 0                    ; smbi_ptr
    dq 0                    ; efi_ptr
    dq 0                    ; mp_ptr
    dq 0                    ; unused0
    dq 0                    ; unused1
    dq 0                    ; unused2
    dq 0                    ; unused3
bootboot_header_end:

;==== Multiboot Header (для обратной совместимости) ====
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
extern bootboot_init

start:
    cli
    
    ; Проверяем, какой загрузчик нас загрузил
    cmp eax, 0x2BADB002      ; Multiboot magic?
    je .multiboot_entry
    cmp dword [0x1000], 'BOOT'  ; BOOTBOOT magic?
    je .bootboot_entry
    
    ; Ни один загрузчик не распознан
    mov esi, no_loader_msg
    call early_print
    jmp .hang

.multiboot_entry:
    mov [multiboot_magic], eax
    mov [multiboot_info], ebx
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
    jmp .hang

.bootboot_entry:
    ; BOOTBOOT загрузчик уже настроил всё за нас
    mov esp, stack_top
    call clear_bss
    
    ; Инициализируем минимальные структуры
    call enable_a20
    call init_gdt
    call init_idt
    call init_pic
    sti
    
    ; Инициализируем BOOTBOOT
    call bootboot_init
    
    mov esi, bootboot_msg
    call early_print
    
    ; Вызываем kernel_main с параметрами BOOTBOOT
    push 0xB007B007          ; BOOTBOOT magic
    push 0x00001000          ; BOOTBOOT info pointer
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

;==== Вспомогательные функции ====
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
    ; Очищаем IDT
    mov edi, idt
    mov ecx, 256
    xor eax, eax
    rep stosd
    
    ; Настраиваем обработчик клавиатуры (IRQ 1)
    mov eax, keyboard_isr
    mov word [idt + 0x21 * 8], ax        ; низкие 16 бит смещения
    mov word [idt + 0x21 * 8 + 2], 0x08  ; селектор
    mov word [idt + 0x21 * 8 + 4], 0x8E00 ; атрибуты (P=1, DPL=0, 32-bit)
    shr eax, 16
    mov word [idt + 0x21 * 8 + 6], ax    ; высокие 16 бит смещения
    
    ; Загружаем IDT
    lidt [idt_descriptor]
    ret

init_pic:
    ; Переинициализация PIC
    mov al, 0x11        ; ICW1: инициализация, ожидание ICW4
    out 0x20, al        ; мастер PIC
    out 0xA0, al        ; ведомый PIC
    
    ; ICW2: смещение векторов прерываний
    mov al, 0x20        ; мастер: IRQ0 -> INT 0x20
    out 0x21, al
    mov al, 0x28        ; ведомый: IRQ8 -> INT 0x28
    out 0xA1, al
    
    ; ICW3: каскадирование
    mov al, 0x04        ; мастер: IRQ2 подключен к ведомому
    out 0x21, al
    mov al, 0x02        ; ведомый: подключен к IRQ2 мастера
    out 0xA1, al
    
    ; ICW4: дополнительные настройки
    mov al, 0x01        ; 8086/88 режим
    out 0x21, al
    out 0xA1, al
    
    ; Маски прерываний
    mov al, 0xFC        ; разрешаем только IRQ1 (клавиатура) и IRQ0 (таймер)
    out 0x21, al
    mov al, 0xFF        ; запрещаем все прерывания ведомого
    out 0xA1, al
    ret

; Обработчик прерывания клавиатуры
keyboard_isr:
    pusha
    cld
    
    ; Читаем скан-код
    in al, 0x60
    movzx eax, al
    
    ; Вызываем обработчик на C
    push eax
    call keyboard_handler
    add esp, 4
    
    ; Посылаем EOI
    mov al, 0x20
    out 0x20, al
    
    popa
    iret

; Простой вывод на экран в текстовом режиме
early_print:
    mov edi, 0xB8000
    mov ah, 0x0F        ; белый текст на чёрном фоне
.print_loop:
    lodsb               ; загружаем следующий символ
    test al, al         ; проверяем на конец строки
    jz .print_done
    stosw               ; записываем символ и атрибут
    jmp .print_loop
.print_done:
    ret

;==== Данные ====
section .data
align 4
boot_msg: db "Bootloader: Multiboot detected", 0
bootboot_msg: db "Bootloader: BOOTBOOT detected", 0
no_loader_msg: db "ERROR: No bootloader detected", 0
multiboot_error_msg: db "ERROR: Invalid Multiboot signature", 0

; Глобальная таблица дескрипторов (GDT)
gdt:
    ; нулевой дескриптор
    dq 0x0000000000000000
    
    ; сегмент кода (0x08)
    dq 0x00CF9A000000FFFF    ; база=0, лимит=0xFFFFF, доступ=0x9A (код, читаемый)
    
    ; сегмент данных (0x10)
    dq 0x00CF92000000FFFF    ; база=0, лимит=0xFFFFF, доступ=0x92 (данные, записываемый)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1     ; лимит GDT
    dd gdt                   ; адрес GDT

; Дескриптор IDT
idt_descriptor:
    dw 256 * 8 - 1          ; лимит IDT
    dd idt                  ; адрес IDT

;==== BSS секция ====
section .bss
align 16
; Стек
stack_bottom:
    resb 16384              ; 16KB стека
stack_top:

; Переменные для Multiboot
global multiboot_info
global multiboot_magic
multiboot_info:
    resd 1                  ; указатель на информацию Multiboot
multiboot_magic:
    resd 1                  ; magic число Multiboot

; Таблица дескрипторов прерываний
align 8
idt:
    resb 256 * 8            ; 256 записей по 8 байт

; Метаданные BSS секции
global bss_start
global bss_end
bss_start:
bss_end:

; Разрешение на исполнение стека
section .note.GNU-stack noalloc noexec nowrite progbits