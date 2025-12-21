#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdint.h>
#include <stdbool.h>
#define MAX_ASM_CODE 2048
#define MAX_MACHINE_CODE 1024
bool assemble_x86(const char *source, uint8_t *output, uint32_t max_size, uint32_t *out_size);
bool execute_asm(const char *source);
void asm_demo_hello_world(void);
extern const char *hello_world_asm;
extern const char *beep_asm;
extern const char *test_registers_asm;
#endif