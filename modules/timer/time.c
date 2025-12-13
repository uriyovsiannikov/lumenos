#include "../modules/timer/timer.h"
#include "../modules/io/io.h"
#include "../libs/string.h"
#include "../libs/print.h"
#include "time.h"
#include "../modules/mt/multitasking.h"
#include "../modules/syslogger/syslogger.h"
int timezone_offset = 0;
uint8_t read_cmos(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
	log_message("Cmos data readed",LOG_INFO);
}
void get_rtc_time(uint8_t* hour, uint8_t* minute, uint8_t* second) {
    uint8_t regB;
    do {
        outb(CMOS_ADDRESS, 0x0A);
    } while (inb(CMOS_DATA) & 0x80);
    *second = read_cmos(0x00);
    *minute = read_cmos(0x02);
    *hour = read_cmos(0x04);
    regB = read_cmos(0x0B);
    if (!(regB & 0x04)) {
        *second = (*second & 0x0F) + ((*second >> 4) * 10);
        *minute = (*minute & 0x0F) + ((*minute >> 4) * 10);
        *hour = (*hour & 0x0F) + ((*hour >> 4) * 10);
    }
    if (!(regB & 0x02)) {
        if (*hour & 0x80) {
            *hour = ((*hour & 0x7F) + 12) % 24;
        }
    }
    int adjusted_hour = *hour + timezone_offset;
    if (adjusted_hour < 0) {
        adjusted_hour += 24;
    } else if (adjusted_hour >= 24) {
        adjusted_hour -= 24;
    }
    *hour = adjusted_hour;
}