#ifndef PANIC_H
#define PANIC_H
#include <stdint.h>
#define PANIC_BG_COLOR 0x00
#define PANIC_FG_COLOR 0x0F
typedef enum {
    PANIC_UNKNOWN = 0,
    PANIC_DIVISION_BY_ZERO,
    PANIC_DEBUG_EXCEPTION,
    PANIC_NMI_INTERRUPT,
    PANIC_BREAKPOINT,
    PANIC_OVERFLOW,
    PANIC_BOUND_RANGE_EXCEEDED,
    PANIC_INVALID_OPCODE,
    PANIC_DEVICE_NOT_AVAILABLE,
    PANIC_DOUBLE_FAULT,
    PANIC_INVALID_TSS,
    PANIC_SEGMENT_NOT_PRESENT,
    PANIC_STACK_SEGMENT_FAULT,
    PANIC_GENERAL_PROTECTION_FAULT,
    PANIC_PAGE_FAULT,
    PANIC_FPU_EXCEPTION,
    PANIC_ALIGNMENT_CHECK,
    PANIC_MACHINE_CHECK,
    PANIC_SIMD_EXCEPTION,
    PANIC_VIRTUALIZATION_EXCEPTION,
    PANIC_KERNEL_PANIC,
    PANIC_OUT_OF_MEMORY,
    PANIC_STACK_OVERFLOW,
    PANIC_NULL_POINTER,
    PANIC_DOUBLE_FREE,
    PANIC_CORRUPTED_HEAP,
    PANIC_FILESYSTEM_ERROR,
    PANIC_DRIVER_ERROR
} panic_code_t;
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;
    uint32_t cr0, cr2, cr3, cr4;
} registers_t;
void panic_init(void);
void panic(panic_code_t code, const char* message, registers_t* regs);
void panic_with_message(const char* format, ...);
void panic_from_isr(uint32_t interrupt, registers_t* regs);
#define KERNEL_PANIC(msg) panic(PANIC_KERNEL_PANIC, msg, NULL)
#define OUT_OF_MEMORY() panic(PANIC_OUT_OF_MEMORY, "Out of memory", NULL)
#endif