#include "idt.h"
#include "io.h"
#include "../modules/syslogger/syslogger.h"
#include "../libs/print.h"
#include <string.h>
void init_pic() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFC); 
    outb(0xA1, 0xFF);
    log_message("Pic init end", LOG_INFO);
}
void setup_idt_gate(struct idt_entry* entry, uint32_t handler) {
    log_message("Trying to setup idt gate", LOG_WARNING);
    entry->base_lo = handler & 0xFFFF;
    entry->base_hi = (handler >> 16) & 0xFFFF;
    entry->sel = 0x08;
    entry->always0 = 0;
    entry->flags = 0x8E;
    log_message("Idt gate setup end", LOG_INFO);
}
void init_idt() {
	print("Initializing idt...",WHITE);
    log_message("Trying to init idt", LOG_WARNING);
    static struct idt_entry idt[256];
    memset(&idt, 0, sizeof(idt));
    setup_idt_gate(&idt[32], (uint32_t)irq0);
    setup_idt_gate(&idt[33], (uint32_t)irq1);
    struct idt_ptr idtp;
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;
    load_idt(&idtp);
    init_pic();
    asm volatile("sti");
    log_message("Idt init end", LOG_INFO);
	print(" [OK]\n",GREEN);
}