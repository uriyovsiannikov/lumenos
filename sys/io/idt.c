#include "idt.h"
#include "../libs/print.h"
#include "../sys/syslogger/syslogger.h"
#include "io.h"
#include <string.h>
static void (*interrupt_handlers[256])(void) = {0};
extern void timer_interrupt_handler(void);
extern void keyboard_handler(void);
void register_interrupt_handler(uint8_t interrupt, void (*handler)(void)) {
  if (interrupt < 256) {
    interrupt_handlers[interrupt] = handler;
    char log_msg[64];
    char int_str[4];
    itoa(interrupt, int_str, 16);
    strcpy(log_msg, "Registered handler for interrupt 0x");
    strcat(log_msg, int_str);
    log_message(log_msg, LOG_INFO);
  }
}
void irq_handler(uint8_t int_no) {
  if (interrupt_handlers[int_no]) {
    interrupt_handlers[int_no]();
  } else {
    char log_msg[64];
    char int_str[4];
    itoa(int_no, int_str, 16);
    strcpy(log_msg, "Unhandled interrupt: 0x");
    strcat(log_msg, int_str);
    log_message(log_msg, LOG_WARNING);
  }
}
void init_pic() {
  outb(0x20, 0x11);
  outb(0xA0, 0x11);
  outb(0x21, 0x20);
  outb(0xA1, 0x28);
  outb(0x21, 0x04);
  outb(0xA1, 0x02);
  outb(0x21, 0x01);
  outb(0xA1, 0x01);
  outb(0x21, 0xFC & ~(1 << 2) & ~(1 << 6));
  outb(0xA1, 0xFF);
  log_message("Pic init end", LOG_INFO);
}
void setup_idt_gate(struct idt_entry *entry, uint32_t handler) {
  entry->base_lo = handler & 0xFFFF;
  entry->base_hi = (handler >> 16) & 0xFFFF;
  entry->sel = 0x08;
  entry->always0 = 0;
  entry->flags = 0x8E;
}
void init_idt() {
  print("Initializing idt...", WHITE);
  log_message("Trying to init idt", LOG_WARNING);
  static struct idt_entry idt[256];
  for (int i = 0; i < 256; i++) {
    idt[i].base_lo = 0;
    idt[i].base_hi = 0;
    idt[i].sel = 0;
    idt[i].always0 = 0;
    idt[i].flags = 0;
  }
  setup_idt_gate(&idt[32], (uint32_t)irq0);
  setup_idt_gate(&idt[33], (uint32_t)irq1);
  setup_idt_gate(&idt[34], (uint32_t)irq2);
  setup_idt_gate(&idt[35], (uint32_t)irq3);
  setup_idt_gate(&idt[36], (uint32_t)irq4);
  setup_idt_gate(&idt[37], (uint32_t)irq5);
  setup_idt_gate(&idt[38], (uint32_t)irq6);
  setup_idt_gate(&idt[39], (uint32_t)irq7);
  setup_idt_gate(&idt[40], (uint32_t)irq8);
  setup_idt_gate(&idt[41], (uint32_t)irq9);
  setup_idt_gate(&idt[42], (uint32_t)irq10);
  setup_idt_gate(&idt[43], (uint32_t)irq11);
  setup_idt_gate(&idt[44], (uint32_t)irq12);
  setup_idt_gate(&idt[45], (uint32_t)irq13);
  setup_idt_gate(&idt[46], (uint32_t)irq14);
  setup_idt_gate(&idt[47], (uint32_t)irq15);
  struct idt_ptr idtp;
  idtp.limit = sizeof(idt) - 1;
  idtp.base = (uint32_t)&idt;
  load_idt(&idtp);
  init_pic();
  register_interrupt_handler(32, timer_interrupt_handler);
  register_interrupt_handler(33, keyboard_handler);
  asm volatile("sti");
  log_message("Idt init end", LOG_INFO);
  print(" [OK]\n", GREEN);
}
