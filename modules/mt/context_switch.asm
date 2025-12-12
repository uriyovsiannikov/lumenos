; SPDX-License-Identifier: GPL-3.0-or-later
; context_switch.asm
section .text
global context_switch
context_switch:
    push ebp
    mov ebp, esp
    pusha
    pushf
    mov eax, [ebp + 8]    ; old_esp
    mov [eax], esp
    mov esp, [ebp + 12]   ; new_esp
    popf
    popa
    pop ebp
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
