#include "speaker.h"
#include "../modules/io/io.h"
#include "../modules/timer/timer.h"
#define SPEAKER_PORT 0x61
#define PIT_PORT 0x42
#define PIT_CMD_PORT 0x43
#define PIT_FREQUENCY 1193180
static uint32_t beep_duration = 0;
static uint32_t beep_start_tick = 0;
static uint32_t current_frequency = 0;
void speaker_play(uint32_t frequency) {
  if (frequency == 0 || frequency > PIT_FREQUENCY)
    return;
  current_frequency = frequency;
  uint32_t divisor = PIT_FREQUENCY / frequency;
  outb(PIT_CMD_PORT, 0xB6);
  outb(PIT_PORT, (uint8_t)(divisor & 0xFF));
  outb(PIT_PORT, (uint8_t)((divisor >> 8) & 0xFF));
  uint8_t tmp = inb(SPEAKER_PORT);
  if (tmp != (tmp | 3)) {
    outb(SPEAKER_PORT, tmp | 3);
  }
}
void speaker_stop(void) {
  uint8_t tmp = inb(SPEAKER_PORT) & 0xFC;
  outb(SPEAKER_PORT, tmp);
  current_frequency = 0;
}
void speaker_beep_nonblocking(uint32_t frequency, uint32_t duration_ms) {
  speaker_play(frequency);
  beep_duration = duration_ms;
  beep_start_tick = get_ticks();
}
void speaker_update(void) {
  if (beep_duration > 0 && current_frequency > 0) {
    uint32_t current_tick = get_ticks();
    if (current_tick >= beep_start_tick) {
      if (current_tick - beep_start_tick >= beep_duration) {
        speaker_stop();
        beep_duration = 0;
      }
    } else {
      uint32_t max_ticks = 0xFFFFFFFF;
      if ((max_ticks - beep_start_tick) + current_tick >= beep_duration) {
        speaker_stop();
        beep_duration = 0;
      }
    }
  }
}
void speaker_beep_blocking(uint32_t frequency, uint32_t duration_ms) {
  speaker_play(frequency);
  timer_wait(duration_ms);
  speaker_stop();
}
void speaker_test(void) {
  uint32_t frequencies[] = {262, 294, 330, 349, 392, 440, 494, 523};
  uint32_t count = sizeof(frequencies) / sizeof(frequencies[0]);
  for (uint32_t i = 0; i < count; i++) {
    speaker_beep_blocking(frequencies[i], 50);
    timer_wait(30);
  }
}
