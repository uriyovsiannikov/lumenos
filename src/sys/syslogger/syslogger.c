#include "syslogger.h"
#include "../../drivers/serial/serial.h"
#include "../../libs/print.h"
#include <stdint.h>
#include <string.h>
struct log_entry {
  char message[MAX_LOG_LENGTH];
  log_level_t level;
  uint32_t timestamp;
};
static struct log_entry log_entries[MAX_LOG_ENTRIES];
static uint32_t log_count = 0;
extern uint32_t get_uptime_seconds();
void log_message(const char *message, log_level_t level) {
  if (log_count >= MAX_LOG_ENTRIES) {
    for (int i = 0; i < MAX_LOG_ENTRIES - 1; i++) {
      log_entries[i] = log_entries[i + 1];
    }
    log_count = MAX_LOG_ENTRIES - 1;
  }
  strncpy(log_entries[log_count].message, message, MAX_LOG_LENGTH - 1);
  SERIAL_PRINT(message);
  SERIAL_PRINT("\n");
  log_entries[log_count].message[MAX_LOG_LENGTH - 1] = '\0';
  log_entries[log_count].level = level;
  log_entries[log_count].timestamp = get_uptime_seconds();
  log_count++;
}
void show_logs() {
  if (log_count == 0) {
    print_info("No log entries");
    return;
  }
  print_info("System logs:");
  for (uint32_t i = 0; i < log_count; i++) {
    print("\n[", WHITE);
    print_dec(log_entries[i].timestamp, GRAY);
    print("] ", WHITE);
    switch (log_entries[i].level) {
    case LOG_INFO:
      print("INFO: ", GREEN);
      break;
    case LOG_WARNING:
      print("WARN: ", YELLOW);
      break;
    case LOG_ERROR:
      print("ERROR: ", RED);
      break;
    }
    print(log_entries[i].message, WHITE);
  }
  print("\n", WHITE);
}
void clear_logs() {
  log_count = 0;
  print_success("Logs cleared");
}
void syslogger_command(const char *args) {
  if (strcmp(args, "show") == 0) {
    show_logs();
  } else if (strcmp(args, "clear") == 0) {
    clear_logs();
  } else {
    print_error("Usage: syslogs <show|clear>");
  }
}
