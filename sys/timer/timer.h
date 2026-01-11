#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
void timer_init(uint32_t freq_hz);
void timer_interrupt_handler(void);
uint32_t get_ticks(void);
uint32_t get_uptime_seconds(void);
uint32_t get_milliseconds(void);
void timer_wait(uint32_t milliseconds);
extern volatile uint32_t ticks;
#endif
