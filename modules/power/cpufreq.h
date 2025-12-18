#ifndef CPUFREQ_H
#define CPUFREQ_H
#include <stdbool.h>
#include <stdint.h>
typedef enum {
  CPU_PSTATE_MIN = 0,
  CPU_PSTATE_BALANCED,
  CPU_PSTATE_MAX
} cpu_pstate_t;
bool cpufreq_supported();
void cpufreq_init();
void cpufreq_set_pstate(cpu_pstate_t pstate);
uint32_t cpufreq_get_current_freq();
uint32_t cpufreq_get_max_freq();
uint32_t cpufreq_get_min_freq();
#endif
