#include "../libs/print.h"
#include "../libs/string.h"
#include "../sys/io/io.h"
#include "../sys/syslogger/syslogger.h"
#include "../sys/timer/time.h"
#include "../sys/timer/timer.h"
void show_clock() {
  uint8_t hour, minute, second;
  get_rtc_time(&hour, &minute, &second);
  char time_str[9];
  time_str[0] = '0' + hour / 10;
  time_str[1] = '0' + hour % 10;
  time_str[2] = ':';
  time_str[3] = '0' + minute / 10;
  time_str[4] = '0' + minute % 10;
  time_str[5] = ':';
  time_str[6] = '0' + second / 10;
  time_str[7] = '0' + second % 10;
  time_str[8] = '\0';
  print("Current time: ", LIGHT_CYAN);
  print(time_str, YELLOW);
  print("\n", WHITE);
}
void set_time(const char *arg) {
  int is_negative = 0;
  const char *num_start = arg;
  if (*num_start == '-') {
    is_negative = 1;
    num_start++;
  } else if (*num_start == '+') {
    num_start++;
  }
  int valid = 1;
  const char *p = num_start;
  while (*p) {
    if (*p < '0' || *p > '9') {
      valid = 0;
      break;
    }
    p++;
  }
  if (!valid || p == num_start) {
    print_error(
        "Invalid timezone format. Usage: settime [+-]N (e.g. settime -5)");
    return;
  }
  int new_offset = 0;
  p = num_start;
  while (*p) {
    new_offset = new_offset * 10 + (*p - '0');
    p++;
  }
  if (is_negative) {
    new_offset = -new_offset;
  }
  if (new_offset >= -12 && new_offset <= 12) {
    timezone_offset = new_offset;
    print_success("Timezone set to UTC");
    if (timezone_offset > 0) {
      putchar('+', WHITE);
      print_dec(timezone_offset, WHITE);
    } else if (timezone_offset < 0) {
      putchar('-', WHITE);
      print_dec(-timezone_offset, WHITE);
    }
    print("\n", WHITE);
  } else {
    print_error("Invalid timezone. Must be between -12 and +12\n");
    log_message("Timezone setup failed", LOG_ERROR);
  }
}
void show_uptime() {
  uint32_t sec = get_uptime_seconds();
  uint32_t hrs = sec / 3600;
  uint32_t min = (sec % 3600) / 60;
  uint32_t rem_sec = sec % 60;
  print("System uptime: ", LIGHT_CYAN);
  if (hrs > 0) {
    print_dec(hrs, WHITE);
    print(hrs == 1 ? " hour " : " hours ", LIGHT_CYAN);
  }
  if (min > 0 || hrs > 0) {
    print_dec(min, WHITE);
    print(min == 1 ? " minute " : " minutes ", LIGHT_CYAN);
  }
  print_dec(rem_sec, WHITE);
  print(rem_sec == 1 ? " second" : " seconds", LIGHT_CYAN);
  print("\n", WHITE);
}
