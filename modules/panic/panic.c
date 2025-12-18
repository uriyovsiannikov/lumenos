#include "panic.h"
#include "../include/stdio.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/syslogger/syslogger.h"
#include <stdarg.h>
static char panic_message[512];
static registers_t panic_registers;
static const char *panic_names[] = {
    "UNKNOWN ERROR",        "DIVISION BY ZERO",
    "DEBUG EXCEPTION",      "NMI INTERRUPT",
    "BREAKPOINT",           "OVERFLOW",
    "BOUND RANGE EXCEEDED", "INVALID OPCODE",
    "DEVICE NOT AVAILABLE", "DOUBLE FAULT",
    "INVALID TSS",          "SEGMENT NOT PRESENT",
    "STACK SEGMENT FAULT",  "GENERAL PROTECTION FAULT",
    "PAGE FAULT",           "FPU EXCEPTION",
    "ALIGNMENT CHECK",      "MACHINE CHECK",
    "SIMD EXCEPTION",       "VIRTUALIZATION EXCEPTION",
    "KERNEL PANIC",         "OUT OF MEMORY",
    "STACK OVERFLOW",       "NULL POINTER DEREFERENCE",
    "DOUBLE FREE DETECTED", "CORRUPTED HEAP",
    "FILESYSTEM ERROR",     "DRIVER ERROR"};
void panic_init(void) {
  log_message("Trying to init panic", LOG_WARNING);
  print("Initializing panic...", WHITE);
  memset(panic_message, 0, sizeof(panic_message));
  memset(&panic_registers, 0, sizeof(panic_registers));
  print(" [OK]\n", GREEN);
  log_message("Panic initialized", LOG_INFO);
}
static void clear_screen_panic(void) {
  for (int y = 0; y < 25; y++) {
    for (int x = 0; x < 80; x++) {
      putchar(' ', PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    }
  }
}
static void print_panic_header(void) {
  print(":( :( :( :( :( :( :( :( ", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("\n\nLumenOS encountered a critical error and needs to restart.\n",
        PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("We're just collecting some error info, and then we'll restart for "
        "you.\n\n",
        PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
}
static void print_error_info(panic_code_t code, const char *message) {
  char buffer[64];
  print("Error Code: ", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  itoa(code, buffer, 10);
  print(buffer, PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print(" (", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print(panic_names[code], PANIC_BG_COLOR << 4 | 0x0E);
  print(")\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  if (message && message[0] != '\0') {
    print("Message: ", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    print(message, PANIC_BG_COLOR << 4 | 0x0C);
    print("\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  }
}
static void print_registers_info(registers_t *regs) {
  if (!regs)
    return;
  print("\nRegister Dump:\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("EAX=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->eax, PANIC_BG_COLOR << 4 | 0x0A);
  print(" EBX=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->ebx, PANIC_BG_COLOR << 4 | 0x0A);
  print(" ECX=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->ecx, PANIC_BG_COLOR << 4 | 0x0A);
  print(" EDX=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->edx, PANIC_BG_COLOR << 4 | 0x0A);
  print("\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("ESI=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->esi, PANIC_BG_COLOR << 4 | 0x0A);
  print(" EDI=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->edi, PANIC_BG_COLOR << 4 | 0x0A);
  print(" EBP=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->ebp, PANIC_BG_COLOR << 4 | 0x0A);
  print(" ESP=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->esp, PANIC_BG_COLOR << 4 | 0x0A);
  print("\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("EIP=", PANIC_BG_COLOR << 4 | 0x0C);
  print_hex(regs->eip, PANIC_BG_COLOR << 4 | 0x0C);
  print(" EFL=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->eflags, PANIC_BG_COLOR << 4 | 0x0A);
  print(" CR2=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print_hex(regs->cr2, PANIC_BG_COLOR << 4 | 0x0A);
  print("\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
}
static void print_stack_trace(registers_t *regs) {
  if (!regs)
    return;
  print("\nStack Trace (EBP chain):\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  uint32_t *ebp = (uint32_t *)regs->ebp;
  int frames = 0;
  while (ebp && frames < 10) {
    uint32_t eip = ebp[1];
    if (eip < 0xC0000000)
      break;
    print("  #", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    char frame_num[4];
    itoa(frames, frame_num, 10);
    print(frame_num, PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    print(" EIP=", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    print_hex(eip, PANIC_BG_COLOR << 4 | 0x0A);
    print("\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    if (!ebp[0] || ebp[0] == (uint32_t)ebp)
      break;
    ebp = (uint32_t *)ebp[0];
    frames++;
  }
}
static void print_system_info(void) {
  print("\nSystem Information:\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("LumenOS v0.3\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  print("Kernel: 1.1\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  extern uint32_t get_total_memory(void);
  uint32_t total_mem = get_total_memory();
  if (total_mem > 0) {
    print("Memory: ", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
    char mem_buf[16];
    itoa(total_mem / 1024, mem_buf, 10);
    print(mem_buf, PANIC_BG_COLOR << 4 | 0x0A);
    print(" MB\n", PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  }
}
static void halt_system(void) {
  log_message("PANIC --- HALTING SYSTEM", LOG_ERROR);
  print("\n\nSystem halted. Press RESET to restart.\n",
        PANIC_BG_COLOR << 4 | PANIC_FG_COLOR);
  asm volatile("cli");
  while (1) {
    asm volatile("hlt");
  }
  log_message("PANIC --- SYSTEM HALTED", LOG_ERROR);
}
void panic(panic_code_t code, const char *message, registers_t *regs) {
  log_message("PANIC --- TRIGGERED", LOG_WARNING);
  asm volatile("cli");
  if (message) {
    strncpy(panic_message, message, sizeof(panic_message) - 1);
  }
  if (regs) {
    memcpy(&panic_registers, regs, sizeof(registers_t));
  }
  clear_screen_panic();
  print_panic_header();
  print_error_info(code, message);
  if (regs) {
    print_registers_info(regs);
    print_stack_trace(regs);
  }
  print_system_info();
  halt_system();
}
void panic_with_message(const char *format, ...) {
  va_list args;
  char buffer[256];
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  panic(PANIC_KERNEL_PANIC, buffer, NULL);
}
void panic_from_isr(uint32_t interrupt, registers_t *regs) {
  panic_code_t code = PANIC_UNKNOWN;
  switch (interrupt) {
  case 0:
    code = PANIC_DIVISION_BY_ZERO;
    break;
  case 1:
    code = PANIC_DEBUG_EXCEPTION;
    break;
  case 2:
    code = PANIC_NMI_INTERRUPT;
    break;
  case 3:
    code = PANIC_BREAKPOINT;
    break;
  case 4:
    code = PANIC_OVERFLOW;
    break;
  case 5:
    code = PANIC_BOUND_RANGE_EXCEEDED;
    break;
  case 6:
    code = PANIC_INVALID_OPCODE;
    break;
  case 7:
    code = PANIC_DEVICE_NOT_AVAILABLE;
    break;
  case 8:
    code = PANIC_DOUBLE_FAULT;
    break;
  case 10:
    code = PANIC_INVALID_TSS;
    break;
  case 11:
    code = PANIC_SEGMENT_NOT_PRESENT;
    break;
  case 12:
    code = PANIC_STACK_SEGMENT_FAULT;
    break;
  case 13:
    code = PANIC_GENERAL_PROTECTION_FAULT;
    break;
  case 14:
    code = PANIC_PAGE_FAULT;
    break;
  case 16:
    code = PANIC_FPU_EXCEPTION;
    break;
  case 17:
    code = PANIC_ALIGNMENT_CHECK;
    break;
  case 18:
    code = PANIC_MACHINE_CHECK;
    break;
  case 19:
    code = PANIC_SIMD_EXCEPTION;
    break;
  case 20:
    code = PANIC_VIRTUALIZATION_EXCEPTION;
    break;
  default:
    code = PANIC_UNKNOWN;
    break;
  }
  panic(code, "Hardware Exception", regs);
}
