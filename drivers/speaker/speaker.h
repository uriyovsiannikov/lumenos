#ifndef SPEAKER_H
#define SPEAKER_H
#include <stdint.h>
void speaker_init(void);
void speaker_play(uint32_t frequency);
void speaker_stop(void);
void speaker_beep_nonblocking(uint32_t frequency, uint32_t duration_ms);
void speaker_update(void);
void speaker_beep_blocking(uint32_t frequency, uint32_t duration_ms);
void speaker_test(void);
#endif