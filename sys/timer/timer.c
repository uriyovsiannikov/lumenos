#include "timer.h"
#include "../io/io.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../syslogger/syslogger.h"
#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
#define PIT_BASE_FREQ 1193180
#define TIMER_HZ 100
#define SCHEDULE_EVERY 2
volatile uint32_t ticks = 0;
static volatile uint32_t seconds = 0;
void timer_interrupt_handler(void) {
  ticks++;
  if (ticks >= TIMER_HZ) {
    seconds++;
    ticks = 0;
  }
  outb(0x20, 0x20);
}
void timer_init(uint32_t freq_hz) {
  print("Initializing timer...", WHITE);
  log_message("Trying to init timer", LOG_WARNING);
  if (freq_hz < 20)
    freq_hz = 20;
  if (freq_hz > 1000)
    freq_hz = 1000;
  uint32_t divisor = PIT_BASE_FREQ / freq_hz;
  outb(PIT_CMD, 0x36);
  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
  ticks = 0;
  seconds = 0;
  log_message("Timer init end", LOG_INFO);
  print(" [OK]\n", GREEN);
}
uint32_t get_ticks() { return ticks; }
uint32_t get_uptime_seconds() { return seconds; }
uint32_t get_milliseconds() {
  uint32_t s = seconds;
  uint32_t t = ticks;
  return (s * 1000) + (t * 1000 / TIMER_HZ);
}
void timer_wait(uint32_t ms) {
  log_message("Trying to pause timer", LOG_WARNING);
  volatile uint32_t count = ms * 200000;
  while (count > 0) {
    count--;
    asm volatile("" ::: "memory");
  }
  log_message("Timer paused", LOG_INFO);
}