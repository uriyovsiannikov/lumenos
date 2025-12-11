; context_switch.asm - УПРОЩЕННАЯ ВЕРСИЯ
section .text
global context_switch

; void context_switch(uint32_t* old_esp, uint32_t new_esp)
context_switch:
    push ebp
    mov ebp, esp
    
    ; Сохраняем контекст текущей задачи
    pusha
    pushf
    
    ; Сохраняем ESP
    mov eax, [ebp + 8]    ; old_esp
    mov [eax], esp
    
    ; Восстанавливаем контекст новой задачи  
    mov esp, [ebp + 12]   ; new_esp
    
    ; Восстанавливаем регистры
    popf
    popa
    pop ebp
    
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
