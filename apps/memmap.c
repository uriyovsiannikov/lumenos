#include "memmap.h"
#include "../libs/print.h"
#include "../libs/string.h"
#include "../modules/syslogger/syslogger.h"
uint32_t* mmap_addr = 0;
uint32_t mmap_length = 0;
uint32_t mmap_entries_count = 0;
void init_memory_map(uint32_t magic, uint32_t* mb_info) {
	log_message("Trying to init memmap",LOG_WARNING);
    if (magic != 0x2BADB002) {
        print("Invalid multiboot magic\n", RED);
		log_message("Invalid magic",LOG_ERROR);
        return;
    }
    uint32_t flags = *mb_info;
    if (flags & (1 << 6)) {
        mmap_length = *(mb_info + 11);
        mmap_addr = (uint32_t*)*(mb_info + 12);
        uint32_t* mmap_ptr = mmap_addr;
        uint32_t mmap_end = (uint32_t)mmap_addr + mmap_length;
        mmap_entries_count = 0;
        while ((uint32_t)mmap_ptr < mmap_end) {
            uint32_t size = *mmap_ptr;
            mmap_ptr = (uint32_t*)((uint32_t)mmap_ptr + size + sizeof(uint32_t));
            mmap_entries_count++;
        }
        print_info("Memory map initialized: ");
        char buffer[16];
        itoa(mmap_entries_count, buffer, 10);
        print(buffer, WHITE);
        print(" entries found", WHITE);
		print(" [OK]\n",GREEN);
		log_message("Memmap init end",LOG_INFO);
    }
}
const char* get_memory_type_string(uint32_t type) {
    switch (type) {
        case MEMORY_AVAILABLE:    return "Available";
        case MEMORY_RESERVED:     return "Reserved";
        case MEMORY_ACPI_RECLAIM: return "ACPI Reclaim";
        case MEMORY_ACPI_NVS:     return "ACPI NVS";
        case MEMORY_BAD:          return "Bad Memory";
        default:                  return "Unknown";
    }
}
uint64_t get_total_memory(void) {
    if (mmap_addr == 0) return 0;
    uint64_t total = 0;
    uint32_t* mmap_ptr = mmap_addr;
    uint32_t mmap_end = (uint32_t)mmap_addr + mmap_length;
    while ((uint32_t)mmap_ptr < mmap_end) {
        uint32_t size = *mmap_ptr;
        uint64_t length = *(mmap_ptr + 3) | ((uint64_t)*(mmap_ptr + 4) << 32);
        total += length;
        mmap_ptr = (uint32_t*)((uint32_t)mmap_ptr + size + sizeof(uint32_t));
    }
    return total;
	log_message("Total memory parsed",LOG_INFO);
}
uint64_t get_available_memory(void) {
    if (mmap_addr == 0) return 0;
    uint64_t available = 0;
    uint32_t* mmap_ptr = mmap_addr;
    uint32_t mmap_end = (uint32_t)mmap_addr + mmap_length;
    while ((uint32_t)mmap_ptr < mmap_end) {
        uint32_t size = *mmap_ptr;
        uint32_t type = *(mmap_ptr + 5);
        uint64_t length = *(mmap_ptr + 3) | ((uint64_t)*(mmap_ptr + 4) << 32);
        if (type == MEMORY_AVAILABLE) {
            available += length;
        }
        mmap_ptr = (uint32_t*)((uint32_t)mmap_ptr + size + sizeof(uint32_t));
    }
    return available;
	log_message("Available memory parsed",LOG_INFO);
}
uint32_t get_memory_map_entries_count(void) {
    return mmap_entries_count;
	log_message("Entries count parsed",LOG_INFO);
}
void show_memory_map(void) {
    print_info("Memory Map:\n");
    if (mmap_addr == 0 || mmap_length == 0) {
        print("  No memory map available from bootloader\n", WHITE);
		log_message("Memmap data parsing error",LOG_ERROR);
        return;
    }
    uint32_t* mmap_ptr = mmap_addr;
    uint32_t mmap_end = (uint32_t)mmap_addr + mmap_length;
    int entry_num = 0;
    print("  GRUB Memory Map (", WHITE);
    char buffer[16];
    itoa(mmap_entries_count, buffer, 10);
    print(buffer, WHITE);
    print(" entries):\n", WHITE);
    while ((uint32_t)mmap_ptr < mmap_end) {
        uint32_t size = *mmap_ptr;
        uint64_t base = *(mmap_ptr + 1) | ((uint64_t)*(mmap_ptr + 2) << 32);
        uint64_t length = *(mmap_ptr + 3) | ((uint64_t)*(mmap_ptr + 4) << 32);
        uint32_t type_correct = *(mmap_ptr + 5);
        uint64_t end_addr = base + length - 1;
        print("    ", WHITE);
        itoa(entry_num, buffer, 10);
        print(buffer, WHITE);
        print(": 0x", WHITE);
        char addr_buf[9];
        for (int i = 7; i >= 0; i--) {
            uint8_t nibble = (base >> (i * 4)) & 0xF;
            addr_buf[7-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        addr_buf[8] = '\0';
        print(addr_buf, WHITE);
        print("-0x", WHITE);
        for (int i = 7; i >= 0; i--) {
            uint8_t nibble = (end_addr >> (i * 4)) & 0xF;
            addr_buf[7-i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        print(addr_buf, WHITE);
        print(" (", WHITE);
        print(get_memory_type_string(type_correct), WHITE);
        print(") - ", WHITE);
        if (length >= 1024 * 1024 * 1024) {
            uint32_t size_gb = (uint32_t)(length / (1024 * 1024 * 1024));
            itoa(size_gb, buffer, 10);
            print(buffer, WHITE);
            print(" GB\n", WHITE);
        } else if (length >= 1024 * 1024) {
            uint32_t size_mb = (uint32_t)(length / (1024 * 1024));
            itoa(size_mb, buffer, 10);
            print(buffer, WHITE);
            print(" MB\n", WHITE);
        } else if (length >= 1024) {
            uint32_t size_kb = (uint32_t)(length / 1024);
            itoa(size_kb, buffer, 10);
            print(buffer, WHITE);
            print(" KB\n", WHITE);
        } else {
            itoa((uint32_t)length, buffer, 10);
            print(buffer, WHITE);
            print(" bytes\n", WHITE);
        }
        mmap_ptr = (uint32_t*)((uint32_t)mmap_ptr + size + sizeof(uint32_t));
        entry_num++;
    }
    print_info("\nMemory Statistics:\n");
    uint64_t total = get_total_memory();
    uint64_t available = get_available_memory();
    uint64_t reserved = total - available;
    print("  Total: ", WHITE);
    itoa(total / (1024 * 1024), buffer, 10);
    print(buffer, WHITE);
    print(" MB\n", WHITE);
    print("  Available: ", WHITE);
    itoa(available / (1024 * 1024), buffer, 10);
    print(buffer, WHITE);
    print(" MB\n", WHITE);
    print("  Reserved: ", WHITE);
    itoa(reserved / (1024 * 1024), buffer, 10);
    print(buffer, WHITE);
    print(" MB\n", WHITE);
}