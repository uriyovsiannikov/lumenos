#include "../libs/print.h"
#include "../modules/io/io.h"
#include "../modules/syslogger/syslogger.h"
#include "../modules/timer/timer.h"
#include <stdint.h>
void reboot() {
  log_message("Trying to reboot", LOG_INFO);
  print_info("\nRebooting system...");
  timer_wait(1000);
  uint8_t temp;
  do {
    temp = inb(0x64);
    if (temp & 0x01) {
      (void)inb(0x60);
    }
  } while (temp & 0x02);
  outb(0x64, 0xFE);
  asm volatile("hlt");
}
void shutdown() {
  log_message("Trying to shutdown", LOG_INFO);
  print_info("System is shutting down...");
  timer_wait(1000);
  outw(0x604, 0x2000);
  outw(0xB004, 0x2000);
  print_info("Power off");
  while (1)
    asm volatile("hlt");
}
