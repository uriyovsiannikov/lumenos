#ifndef IDT_H
#define IDT_H
#include <stdint.h>
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));
extern void irq0(void);
extern void irq1(void);
extern void load_idt(struct idt_ptr* idtp);
void init_idt(void);
void setup_idt_gate(struct idt_entry* entry, uint32_t handler);
void init_pic(void);
#endif