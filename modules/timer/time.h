#ifndef TIME_H
#define TIME_H
#include <stdint.h>
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71
extern int timezone_offset;
uint8_t read_cmos(uint8_t reg);
void get_rtc_time(uint8_t* hour, uint8_t* minute, uint8_t* second);
#endif 