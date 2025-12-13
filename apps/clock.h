#ifndef CLOCK_H
#define CLOCK_H
#include <stdint.h>
void show_clock(void);
void set_time(const char* arg);
void show_uptime(void);
extern int timezone_offset;
#endif 