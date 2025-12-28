#include "cpufreq.h"
#include "../../libs/print.h"
#include "../io/io.h"
#define MSR_IA32_PERF_STATUS 0x198
#define MSR_IA32_PERF_CTL 0x199
#define MSR_IA32_APERF 0xE8
#define MSR_IA32_MPERF 0xE7
#define MSR_IA32_MISC_ENABLE 0x1A0
static bool supports_msr() {
  uint32_t eax, edx;
  asm volatile("cpuid" : "=a"(eax), "=d"(edx) : "a"(1) : "ecx", "ebx");
  return (edx & (1 << 5));
}
static uint64_t rdmsr(uint32_t msr) {
  if (!supports_msr()) {
    return 0;
  }
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}
static void wrmsr(uint32_t msr, uint64_t value) {
  if (!supports_msr()) {
    return;
  }
  uint32_t low = (uint32_t)(value & 0xFFFFFFFF);
  uint32_t high = (uint32_t)(value >> 32);
  asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}
bool cpufreq_supported() {
  if (!supports_msr()) {
    return false;
  }
  uint64_t misc_enable = rdmsr(MSR_IA32_MISC_ENABLE);
  if (misc_enable & (1ULL << 16)) {
    return false;
  }
  return true;
}
void cpufreq_init() {
  if (!cpufreq_supported()) {
    print("CPU frequency control not supported", RED);
    return;
  }
  print("CPU frequency control initialized", GREEN);
}
void cpufreq_set_pstate(cpu_pstate_t pstate) {
  if (!cpufreq_supported()) {
    return;
  }
  uint32_t multiplier;
  switch (pstate) {
  case CPU_PSTATE_MIN:
    multiplier = 8;
    break;
  case CPU_PSTATE_BALANCED:
    multiplier = 25;
    break;
  case CPU_PSTATE_MAX:
    multiplier = 40;
    break;
  default:
    multiplier = 25;
  }
  uint64_t new_ctl = (multiplier << 8);
  wrmsr(MSR_IA32_PERF_CTL, new_ctl);
}
uint32_t cpufreq_get_current_freq() {
  if (!cpufreq_supported()) {
    return 0;
  }
  static uint32_t last_freq = 0;
  return last_freq > 0 ? last_freq : 2500;
}
uint32_t cpufreq_get_max_freq() { return 4000; }
uint32_t cpufreq_get_min_freq() { return 800; }
