#include <stdint.h>
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/timer/timer.h"
#include "../modules/mm/mm.h"
#include "../modules/syslogger/syslogger.h"
extern volatile uint32_t timer_ticks;
extern uint32_t* mmap_addr;
extern uint32_t mmap_length;
static void safe_itoa(uint32_t value, char* buffer, int base) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    char temp[32];
    int i = 0;
    while (value > 0) {
        uint32_t digit = value % base;
        temp[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value /= base;
    }
    for (int j = 0; j < i; j++) {
        buffer[j] = temp[i - j - 1];
    }
    buffer[i] = '\0';
}
void show_system_info() {
    log_message("Trying to parse some sysinfo", LOG_WARNING);
    uint32_t eax, ebx, ecx, edx;
    char buffer[64];
    print_info("=== System Information ===\n\n");
    print_info("CPU Information:\n");
    print("  Vendor: ", WHITE);
    char vendor[13];
    asm volatile("cpuid" 
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
        : "a"(0));
    *((uint32_t*)vendor) = ebx;
    *((uint32_t*)(vendor+4)) = edx;
    *((uint32_t*)(vendor+8)) = ecx;
    vendor[12] = '\0';
    print(vendor, WHITE);
    print("\n", WHITE);
    asm volatile("cpuid" 
        : "=a"(eax) 
        : "a"(0x80000000));
    if (eax >= 0x80000004) {
        char brand[49] = {0};
        for (int i = 0; i < 3; i++) {
            asm volatile("cpuid"
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                : "a"(0x80000002 + i));
            *((uint32_t*)(brand + i*16)) = eax;
            *((uint32_t*)(brand + i*16 + 4)) = ebx;
            *((uint32_t*)(brand + i*16 + 8)) = ecx;
            *((uint32_t*)(brand + i*16 + 12)) = edx;
        }
        print("  Brand: ", WHITE);
        print(brand, WHITE);
        print("\n", WHITE);
    }
    asm volatile("cpuid" 
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) 
        : "a"(1));
    print("  Features: ", WHITE);
    if (edx & (1 << 23)) print("MMX ", WHITE);
    if (edx & (1 << 25)) print("SSE ", WHITE);
    if (edx & (1 << 26)) print("SSE2 ", WHITE);
    if (edx & (1 << 28)) print("HTT ", WHITE);
    if (ecx & (1 << 0)) print("SSE3 ", WHITE);
    if (ecx & (1 << 9)) print("SSSE3 ", WHITE);
    if (ecx & (1 << 19)) print("SSE4.1 ", WHITE);
    if (ecx & (1 << 20)) print("SSE4.2 ", WHITE);
    if (ecx & (1 << 23)) print("POPCNT ", WHITE);
    if (ecx & (1 << 28)) print("AVX ", WHITE);
    print("\n", WHITE);
    print("  Cores: ", WHITE);
    asm volatile("cpuid" 
        : "=a"(eax) 
        : "a"(0x0B));
    if (eax >= 0x0B) {
        uint32_t logical_cores = 1;
        uint32_t physical_cores = 1;
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0x0B), "c"(0));
        logical_cores = ebx & 0xFFFF;
        asm volatile("cpuid"
            : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(0x0B), "c"(1));
        physical_cores = ebx & 0xFFFF;
        safe_itoa(physical_cores, buffer, 10);
        print(buffer, WHITE);
        print(" physical, ", WHITE);
        safe_itoa(logical_cores, buffer, 10);
        print(buffer, WHITE);
        print(" logical\n", WHITE);
    } else {
        print("1 (legacy CPU)\n", WHITE);
    }
    print("  Frequency: ~??? MHz (Parcing error)\n", WHITE);
    print("\n", WHITE);
    print_info("Memory Information:\n");
    uint64_t total_memory = 128 * 1024 * 1024; 
    if (mmap_addr && mmap_length > 0) {
        uint32_t* mmap_ptr = mmap_addr;
        uint32_t mmap_end = (uint32_t)mmap_addr + mmap_length;
        total_memory = 0;
        while ((uint32_t)mmap_ptr + 24 <= mmap_end) {
            uint32_t size = *mmap_ptr;
            uint32_t type = *(mmap_ptr + 4);
            if (type == 1) { 
                uint64_t base = *(mmap_ptr + 1);
                uint64_t length = *(mmap_ptr + 3);
                total_memory += length;
            }
            mmap_ptr = (uint32_t*)((uint32_t)mmap_ptr + size + 4);
            if (size == 0 || (uint32_t)mmap_ptr >= mmap_end) break;
        }
        if (total_memory == 0) {
            total_memory = 128 * 1024 * 1024;
        }
    }
    print("  Total RAM: ", WHITE);
    safe_itoa((uint32_t)(total_memory / (1024 * 1024)), buffer, 10);
    print(buffer, WHITE);
    print(" MB\n", WHITE);
    print("  Heap size: ", WHITE);
    safe_itoa(HEAP_SIZE / (1024 * 1024), buffer, 10);
    print(buffer, WHITE);
    print(" MB\n", WHITE);
    print("\n", WHITE);
    print_info("Graphics Information:\n");
    print("  VGA Mode: 80x25 text mode\n", WHITE);
    print("  Color depth: 16 colors\n", WHITE);
    print("\n", WHITE);
    print_info("System Status:\n");
    uint32_t uptime_seconds = 0;
    if (get_uptime_seconds) {
        uptime_seconds = get_uptime_seconds();
    } else {
        uptime_seconds = timer_ticks / 100; 
    }
    uint32_t hours = uptime_seconds / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    uint32_t seconds_remaining = uptime_seconds % 60;
    print("  Uptime: ", WHITE);
    safe_itoa(hours, buffer, 10);
    print(buffer, WHITE);
    print("h ", WHITE);
    safe_itoa(minutes, buffer, 10);
    print(buffer, WHITE);
    print("m ", WHITE);
    safe_itoa(seconds_remaining, buffer, 10);
    print(buffer, WHITE);
    print("s\n", WHITE);
    print("  Kernel: LumenOS v0.1\n", WHITE);
    print("  Architecture: i386 (32-bit)\n", WHITE);
    print("\n", WHITE);
    print_info("Hardware Features:\n");
    uint16_t fpu_test = 0xFFFF;
    asm volatile(
        "fninit\n"
        "fnstsw %0\n"
        : "=m"(fpu_test)
    );
    if (fpu_test == 0) {
        print("  FPU: Present\n", WHITE);
    } else {
        print("  FPU: Not present\n", WHITE);
    }
    asm volatile("cpuid" : "=d"(edx) : "a"(1));
    if (edx & (1 << 9)) {
        print("  APIC: Present\n", WHITE);
    } else {
        print("  APIC: Not present\n", WHITE);
    }
    print_info("=== End of System Information ===\n");
    log_message("System information displayed", LOG_INFO);
}